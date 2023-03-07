/**
 * \ingroup pid
 * 
 * \file pid.c
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief PID Library source
 * \version 0.2
 * \date 2022-08-16
*/

#include "utils/pid.h"

#include <stdlib.h>
#include <string.h>


//#define PID_PRINT_DEBUG_MESSAGES
#ifdef PID_PRINT_DEBUG_MESSAGES
#include <stdio.h>
#endif

#define DISCRETE_DERIVATIVE_SHIFT_AT_VAL 0x00FFFFFF

/**
 * \brief Stuct representing a discrete derivative.
 * 
 * A discrete derivative is essentially the slope of best fit for the sequence of most recent datapoints
 * in a time series. All points measured within some number of ms are used to create this slope of best
 * fit.
 */
typedef struct discrete_derivative_s{
    uint16_t filter_span_ms; /**< The amount of the data series in ms that the slope will be fitted to. */
    uint16_t sample_rate_ms; /**< The minimal length of time between datapoints. */
    datapoint * data;        /**< Circular buffer representing the recent datapoints. */
    uint16_t buf_len;        /**< The max number of datapoints the data can hold. */
    uint16_t num_el;         /**< Number of datapoints in buffer. */
    uint16_t start_idx;      /**< The index of the first datapoint in array. */
    datapoint origin;        /**< The current origin of the data. As data/time grows, the origin will be shifted. */
    int64_t sum_v;           /**< The sum of the values in the current datapoints */
    int64_t sum_t;           /**< The sum of the times in the current datapoints */
    int64_t sum_vt;          /**< The sum of the value/time product in the current datapoints */
    int64_t sum_tt;          /**< The sum of the time squared in the current datapoints */
} discrete_derivative_;

/**
 * \brief Stuct representing a discrete integral.
 * 
 * A discrete integral is the area under the curve of a series of datapoints. The area is computed
 * by averaging the value between current and previous datapoint and multiplying that by the duration
 * between the two datapoints. An effect of this math is the area under the curve is 0 until two data
 * points have been passed to it.
 */
typedef struct discrete_integral_s{
    pid_data_t sum;         /**< The current area under the curve. */
    pid_data_t lower_bound; /**< The lower bound on the area under the curve. Useful for anti-windup. */
    pid_data_t upper_bound; /**< The upper bound on the area under the curve. Useful for anti-windup. */
    datapoint prev_p;       /**< The previous datapoint. Updated each time the integral is ticked. */
} discrete_integral_;

/**
 * \brief Struct representing a PID object.
 */
typedef struct pid_s{
    pid_data_t setpoint;                /**< The current setpoint the PID is regulating to. */
    pid_gains K;                        /**< The gains of the PID controller. */
    sensor_getter read_fb;              /**< The sensor function. */
    sensor_getter read_ff;              /**< The sensor providing feedforward values. */
    input_setter apply_input;           /**< The plant function that applies the input to the system. */
    discrete_derivative err_slope;      /**< A discrete_derivative tracking the slope of the error. */
    discrete_integral err_sum;          /**< A discrete_integral tracking the sum of the error. */
    uint16_t min_time_between_ticks_ms; /**< The minimum time, in ms, between ticks. If time hasn't elapsed, the previous input is returned. */
    absolute_time_t _next_tick_time;    /**< Timestamp used to track the earliest time that the system can be ticked again. */
    float last_input;                   /**< Saves the last computed input between while dwelling between ticks. */
} pid_;

inline pid_time_t ms_since_boot(){
    return to_ms_since_boot(get_absolute_time());
}

/** \brief Remove the point in the buffer indicated by the starting index. */
static inline void _discrete_derivative_remove_start_point(discrete_derivative d){
    if(d->num_el > 0){
        d->sum_v -= d->data[d->start_idx].v;
        d->sum_t -= d->data[d->start_idx].t;
        d->sum_vt -= d->data[d->start_idx].t*d->data[d->start_idx].v;
        d->sum_tt -= d->data[d->start_idx].t*d->data[d->start_idx].t;

        d->start_idx = (d->start_idx+1) % d->buf_len;
        d->num_el -= 1;
    }
}

/**
 * \brief Returns a pointer to the element some distance from the starting datapoint.
 * 
 * \param d The discrete derivative to examine.
 * \param offset The number of datapoints after the starting index to return.
 * \return A pointer to the datapoint in the circular array.
 */
static inline datapoint * _discrete_derivative_dp(const discrete_derivative d, const uint16_t offset){
    return &d->data[(d->start_idx + offset) % d->buf_len];
}

/**
 * \brief Returns a pointer to the next open space in the circular datapoint buffer.
 * 
 * \param d The discrete derivative to examine.
 * \return A pointer to the next open space in the circular array.
 */
static inline datapoint * _discrete_derivative_next_dp(const discrete_derivative d){
    return _discrete_derivative_dp(d, d->num_el);
}

/**
 * \brief Returns a pointer to the last element in the circular datapoint buffer.
 * 
 * \param d The discrete derivative to examine.
 * \return A pointer to the datapoint in the circular array. NULL if no datapoints.
 */
static inline datapoint * _discrete_derivative_latest_dp(const discrete_derivative d){
    return (d->num_el>0) ? _discrete_derivative_dp(d, d->num_el - 1) : NULL;
}

/** 
 * \brief Adds a datapoint to the internal data series of d.
 * 
 * Internal summations are also incremented based on the new data.
 */
static inline void _discrete_derivative_add_point(discrete_derivative d, const datapoint p){
    datapoint * new_dp = _discrete_derivative_next_dp(d);
    
    // Set datapoint values with origin offset
    new_dp->v = p.v - d->origin.v;
    new_dp->t = p.t - d->origin.t;

    // Add new values to summations
    d->sum_v += new_dp->v;
    d->sum_t += new_dp->t;
    d->sum_vt += (new_dp->t)*(new_dp->v);
    d->sum_tt += (new_dp->t)*(new_dp->t);

    d->num_el += 1;
}

/**
 * \brief Removes all points in d's data buffer with timestamps more than
 * d->filter_span_ms milliseconds behind cur_t. 
 * 
 * This function will never remove points if only two are left.
 * 
 * \param d Pointer to a \ref discrete_derivative that will be cleaned
 * \param cur_t The current timestamp as milliseconds-since-boot
 */
static void _discrete_derivative_remove_old_points(discrete_derivative d, const uint64_t cur_t) {
    while(d->num_el > 2 && ((cur_t - d->origin.t) - d->data[d->start_idx].t) > d->filter_span_ms){
        _discrete_derivative_remove_start_point(d);
    }
}

/** \brief Doubles the size of the data buffer in d. 
 * 
 * \param d Pointer to discrete_derivative owning buffer to expand.
*/
static void _discrete_derivative_expand_buf(discrete_derivative d) {
    // Get current layout of data in buffer
    const bool wraps = (d->start_idx + d->num_el > d->buf_len);
    const uint16_t len_from_0 = (wraps ? (d->start_idx + d->num_el) % d->buf_len : 0);
    const uint16_t len_from_start = d->num_el - len_from_0;
    
    // Double buf_len and make new buffer
    d->buf_len <<= 1;
    datapoint * new_buf = malloc(sizeof(datapoint) * d->buf_len);
    
    // Copy the data into new buffer
    memcpy(new_buf, &(d->data[d->start_idx]), len_from_start * sizeof(datapoint));
    if(len_from_0 > 0){
        memcpy(&(new_buf[len_from_start]), d->data, len_from_0 * sizeof(datapoint));
    }

    // Free mem and update internal vars to new buffer
    free(d->data);
    d->data = new_buf;
    d->start_idx = 0;
}

/**
 * \brief Shift the time and value data to the origin to avoid overflow.
 * 
 * \param d Discrete Derivative tht will be shifted.
 */
static void _discrete_derivative_shift_data(discrete_derivative d){
    const bool need_to_shift_time = (d->data[d->start_idx].t - d->origin.t > DISCRETE_DERIVATIVE_SHIFT_AT_VAL);
    const bool need_to_shift_val = (d->data[d->start_idx].v - d->origin.v > DISCRETE_DERIVATIVE_SHIFT_AT_VAL);
    if(need_to_shift_time || need_to_shift_val){
        // Get current value of oldest datapoint
        const datapoint shift_amount = d->data[d->start_idx];

        // Shift linear summations and re-init quadratic terms
        d->origin.t += shift_amount.t;
        d->origin.v += shift_amount.v;
        d->sum_t -= shift_amount.t*(d->num_el);
        d->sum_v -= shift_amount.v*(d->num_el);
        d->sum_vt = 0;
        d->sum_tt = 0;

        // Shift all data and compute quadratic terms
        for(uint i = 0; i < d->num_el; i++){
            const uint idx = (d->start_idx + i) % d->buf_len;
            d->data[idx].t -= shift_amount.t;
            d->data[idx].v -= shift_amount.v;
            d->sum_vt += (d->data[idx].t)*(d->data[idx].v);
            d->sum_tt += (d->data[idx].t)*(d->data[idx].t);
        }
    }
}

discrete_derivative discrete_derivative_setup(const uint filter_span_ms, const uint sample_rate_ms) {
    discrete_derivative d = malloc(sizeof(discrete_derivative_));
    discrete_derivative_reset(d);
    d->sample_rate_ms = sample_rate_ms;
    d->filter_span_ms = filter_span_ms;
    d->origin.t = 0;
    d->origin.v = 0;
    d->buf_len = 16;
    d->data = malloc((d->buf_len) * sizeof(datapoint));
    return d;
}

void discrete_derivative_reset(discrete_derivative d) { 
    d->start_idx = 0;
    d->num_el = 0; 
    d->origin.t = 0;
    d->origin.v = 0;
    d->sum_v = 0;
    d->sum_t = 0;
    d->sum_vt = 0;
    d->sum_tt = 0;
}

void discrete_derivative_deinit(discrete_derivative d) {
    free(d->data);
    free(d);
}

float discrete_derivative_read(discrete_derivative d) {
    _discrete_derivative_remove_old_points(d, ms_since_boot());
    if (d->num_el < 2) return 0;
    return (float)(d->sum_vt*d->num_el - (d->sum_t)*(d->sum_v))/(float)(d->sum_tt*d->num_el - (d->sum_t)*(d->sum_t));
}

void discrete_derivative_add_datapoint(discrete_derivative d, const datapoint p) {
    const bool dwell_time_up = p.t - _discrete_derivative_latest_dp(d)->t >= d->sample_rate_ms;
    if(d->num_el == 0 || dwell_time_up){
        _discrete_derivative_remove_old_points(d, p.t);
        if (d->num_el == d->buf_len) _discrete_derivative_expand_buf(d);
        _discrete_derivative_add_point(d, p);
        _discrete_derivative_shift_data(d);
    }
}

void discrete_derivative_add_value(discrete_derivative d, const pid_data_t v){
    const datapoint dp = {.v = v, .t = ms_since_boot()};
    discrete_derivative_add_datapoint(d, dp);
}

discrete_integral discrete_integral_setup(const pid_data_t lower_bound, const pid_data_t upper_bound){
    discrete_integral i = malloc(sizeof(discrete_integral_));
    discrete_integral_reset(i);
    // scale by 2 since the average values are summed, to be devided once when read.
    i->lower_bound = 2*lower_bound;
    i->upper_bound = 2*upper_bound;
    return i;
}

pid_data_t discrete_integral_read(const discrete_integral i) { 
    return i->sum/2.0;
}

void discrete_integral_add_datapoint(discrete_integral i, datapoint p) {
    if (i->prev_p.t != -1) {
        i->sum += (p.v + i->prev_p.v) * (p.t - i->prev_p.t);
        i->sum = (i->sum < i->lower_bound ? i->lower_bound : i->sum);
        i->sum = (i->sum > i->upper_bound ? i->upper_bound : i->sum);
    }
    i->prev_p = p;
}

void discrete_integral_reset(discrete_integral i) {
    i->prev_p.t = -1;
    i->prev_p.v = 0;
    i->sum = 0;
}

void discrete_integral_deinit(discrete_integral i){
    free(i);
}

pid pid_setup(const pid_gains K, sensor_getter feedback_sensor, sensor_getter feedforward_sensor, 
              input_setter plant, const uint16_t time_between_ticks_ms, const pid_data_t windup_lb, 
              const pid_data_t windup_ub, const uint derivative_filter_span_ms){
    pid controller = malloc(sizeof(pid_));
    controller->K = K;
    controller->read_fb = feedback_sensor;
    controller->read_ff = feedforward_sensor;
    controller->apply_input = plant;
    controller->min_time_between_ticks_ms = time_between_ticks_ms;
    controller->setpoint = 0;
    controller->_next_tick_time = get_absolute_time();

    controller->err_sum = discrete_integral_setup(windup_lb, windup_ub);
    controller->err_slope = discrete_derivative_setup(derivative_filter_span_ms, time_between_ticks_ms/2);

    return controller;
}

void pid_update_setpoint(pid controller, const pid_data_t setpoint){
    controller->setpoint = setpoint;
}

float pid_tick(pid controller){
    if(absolute_time_diff_us(get_absolute_time(), controller->_next_tick_time) < 0){
        controller->_next_tick_time = delayed_by_ms(get_absolute_time(), controller->min_time_between_ticks_ms);
        const datapoint new_reading = {.t = ms_since_boot(), .v = controller->read_fb()};
        const datapoint new_err = {.t = new_reading.t, .v = controller->setpoint - new_reading.v};

        discrete_integral_add_datapoint(controller->err_sum, new_err);
        discrete_derivative_add_datapoint(controller->err_slope, new_reading);
        
        const pid_data_t e_sum   = (controller->K.i == 0 ? 0 : discrete_integral_read(controller->err_sum));
        const pid_data_t e_slope = (controller->K.d == 0 ? 0 : discrete_derivative_read(controller->err_slope));
        const pid_data_t ff = (controller->read_ff != NULL ? controller->read_ff() : 0);

        float input = (controller->K.p)*new_err.v + (controller->K.i)*e_sum + (controller->K.d)*e_slope + (controller->K.f)*ff;
        
        #ifdef PID_PRINT_DEBUG_MESSAGES
        printf("%07ld - %07ld - %07ld - %07ld - %07ld - %0.2f\n", new_reading.v, new_err.v, e_sum, e_slope, ff, input);
        #endif

        if(controller->apply_input != NULL) controller->apply_input(input);
        controller->last_input = input;
        return input;
    }
    return controller->last_input;
}

bool pid_at_setpoint(const pid controller, const pid_data_t tol){
    const pid_data_t err = controller->read_fb() - controller->setpoint;
    return (err >= -tol && err <= tol);
}

void pid_reset(pid controller){
    discrete_derivative_reset(controller->err_slope);
    discrete_integral_reset(controller->err_sum);
}

void pid_deinit(pid controller){
    discrete_derivative_deinit(controller->err_slope);
    discrete_integral_deinit(controller->err_sum);
    free(controller);
}