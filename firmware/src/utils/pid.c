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

#define DISCRETE_DERIVATIVE_SHIFT_AT_VAL 0x00FFFFFF

inline pid_time_t ms_since_boot(){
    return to_ms_since_boot(get_absolute_time());
}

/** \brief Remove the point in the buffer indicated by the starting index. */
static inline void _discrete_derivative_remove_start_point(discrete_derivative *d){
    d->_sum_v -= d->_data[d->_start_idx].v;
    d->_sum_t -= d->_data[d->_start_idx].t;
    d->_sum_vt -= d->_data[d->_start_idx].t*d->_data[d->_start_idx].v;
    d->_sum_tt -= d->_data[d->_start_idx].t*d->_data[d->_start_idx].t;

    d->_start_idx = (d->_start_idx+1) % d->_buf_len;
    d->_num_el -= 1;
}

/**
 * \brief Returns a pointer to the element some distance from the starting datapoint.
 * 
 * 
 * \param d The discrete derivative to examine.
 * \param offset The number of datapoints after the starting index to return.
 * \return A pointer to the datapoint in the circular array.
 */
static inline datapoint * _discrete_derivative_dp(discrete_derivative *d, uint16_t offset){
    return &d->_data[(d->_start_idx + offset) % d->_buf_len];
}

/**
 * \brief Returns a pointer to the next open space in the circular datapoint buffer.
 * 
 * \param d The discrete derivative to examine.
 * \return A pointer to the next open space in the circular array.
 */
static inline datapoint * _discrete_derivative_next_dp(discrete_derivative *d){
    return _discrete_derivative_dp(d, d->_num_el);
}

/**
 * \brief Returns a pointer to the last element in the circular datapoint buffer.
 * 
 * \param d The discrete derivative to examine.
 * \return A pointer to the datapoint in the circular array. NULL if no datapoints.
 */
static inline datapoint * _discrete_derivative_latest_dp(discrete_derivative *d){
    return (d->_num_el>0) ? _discrete_derivative_dp(d, d->_num_el - 1) : NULL;
}

/** 
 * \brief Adds a datapoint to the internal data series of d.
 * 
 * Internal summations are also incremented based on the new data.
 */
static inline void _discrete_derivative_add_point(discrete_derivative *d, const datapoint p){
    datapoint * new_dp = _discrete_derivative_next_dp(d);
    
    // Set datapoint values with origin offset
    new_dp->v = p.v - d->_origin.v;
    new_dp->t = p.t - d->_origin.t;

    // Add new values to summations
    d->_sum_v += new_dp->v;
    d->_sum_t += new_dp->t;
    d->_sum_vt += (new_dp->t)*(new_dp->v);
    d->_sum_tt += (new_dp->t)*(new_dp->t);

    d->_num_el += 1;
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
static void _discrete_derivative_remove_old_points(discrete_derivative *d, const uint64_t cur_t) {
    while(d->_num_el > 2 && ((cur_t - d->_origin.t) - d->_data[d->_start_idx].t) > d->filter_span_ms){
        _discrete_derivative_remove_start_point(d);
    }
}

/** \brief Doubles the size of the _data buffer in d. 
 * 
 * \param d Pointer to discrete_derivative owning buffer to expand.
*/
static void _discrete_derivative_expand_buf(discrete_derivative *d) {
    // Get current layout of data in buffer
    const bool wraps = (d->_start_idx + d->_num_el > d->_buf_len);
    const uint16_t len_from_0 = (wraps ? (d->_start_idx + d->_num_el) % d->_buf_len : 0);
    const uint16_t len_from_start = d->_num_el - len_from_0;
    
    // Double _buf_len and make new buffer
    d->_buf_len <<= 1;
    datapoint * new_buf = malloc(sizeof(datapoint) * d->_buf_len);
    
    // Copy the data into new buffer
    memcpy(new_buf, &(d->_data[d->_start_idx]), len_from_start * sizeof(datapoint));
    if(len_from_0 > 0){
        memcpy(&(new_buf[len_from_start]), d->_data, len_from_0 * sizeof(datapoint));
    }

    // Free mem and update internal vars to new buffer
    free(d->_data);
    d->_data = new_buf;
    d->_start_idx = 0;
}

/**
 * \brief Shift the time and value data to the origin to avoid overflow.
 * 
 * \param d Discrete Derivative tht will be shifted.
 */
static void _discrete_derivative_shift_data(discrete_derivative *d){
    const bool need_to_shift_time = (d->_data[d->_start_idx].t - d->_origin.t > DISCRETE_DERIVATIVE_SHIFT_AT_VAL);
    const bool need_to_shift_val = (d->_data[d->_start_idx].v - d->_origin.v > DISCRETE_DERIVATIVE_SHIFT_AT_VAL);
    if(need_to_shift_time || need_to_shift_val){
        // Get current value of oldest datapoint
        const datapoint shift_amount = d->_data[d->_start_idx];

        // Shift linear summations and re-init quadratic terms
        d->_origin.t += shift_amount.t;
        d->_origin.v += shift_amount.v;
        d->_sum_t -= shift_amount.t*(d->_num_el);
        d->_sum_v -= shift_amount.v*(d->_num_el);
        d->_sum_vt = 0;
        d->_sum_tt = 0;

        // Shift all data and compute quadratic terms
        for(uint i = 0; i < d->_num_el; i++){
            const uint idx = (d->_start_idx + i) % d->_buf_len;
            d->_data[idx].t -= shift_amount.t;
            d->_data[idx].v -= shift_amount.v;
            d->_sum_vt += (d->_data[idx].t)*(d->_data[idx].v);
            d->_sum_tt += (d->_data[idx].t)*(d->_data[idx].t);
        }
    }
}

void discrete_derivative_setup(discrete_derivative *d, uint filter_span_ms, uint ms_between_datapoints) {
    discrete_derivative_reset(d);
    d->ms_between_datapoints = ms_between_datapoints;
    d->filter_span_ms = filter_span_ms;
    d->_origin.t = 0;
    d->_origin.v = 0;
    d->_buf_len = 16;
    d->_data = malloc((d->_buf_len) * sizeof(datapoint));
}

void discrete_derivative_reset(discrete_derivative *d) { 
    d->_start_idx = 0;
    d->_num_el = 0; 
    d->_origin.t = 0;
    d->_origin.v = 0;
    d->_sum_v = 0;
    d->_sum_t = 0;
    d->_sum_vt = 0;
    d->_sum_tt = 0;
}

void discrete_derivative_deinit(discrete_derivative *d) {
    d->_buf_len = 0;
    free(d->_data);
}

float discrete_derivative_read(discrete_derivative *d) {
    _discrete_derivative_remove_old_points(d, ms_since_boot());
    if (d->_num_el < 2) return 0;
    return (float)(d->_sum_vt*d->_num_el - (d->_sum_t)*(d->_sum_v))/(float)(d->_sum_tt*d->_num_el - (d->_sum_t)*(d->_sum_t));
}

void discrete_derivative_add_datapoint(discrete_derivative *d, datapoint p) {
    const bool dwell_time_up = p.t - _discrete_derivative_latest_dp(d)->t >= d->ms_between_datapoints;
    if(d->_num_el == 0 || dwell_time_up){
        _discrete_derivative_remove_old_points(d, p.t);
        if (d->_num_el == d->_buf_len) _discrete_derivative_expand_buf(d);
        _discrete_derivative_add_point(d, p);
        _discrete_derivative_shift_data(d);
    }
}

void discrete_derivative_add_value(discrete_derivative* d, pid_data_t v){
    const datapoint dp = {.v = v, .t = ms_since_boot()};
    discrete_derivative_add_datapoint(d, dp);
}

void discrete_integral_setup(discrete_integral *i, const pid_data_t lower_bound, const pid_data_t upper_bound) {
    discrete_integral_reset(i);
    i->lower_bound = lower_bound;
    i->upper_bound = upper_bound;
}

pid_data_t discrete_integral_read(discrete_integral *i) { 
    return i->sum/2.0;
}

void discrete_integral_add_datapoint(discrete_integral *i, datapoint p) {
    if (i->prev_p.t != -1) {
        i->sum += (p.v + i->prev_p.v) * (p.t - i->prev_p.t);
        i->sum = (i->sum < i->lower_bound ? i->lower_bound : i->sum);
        i->sum = (i->sum > i->upper_bound ? i->upper_bound : i->sum);
    }
    i->prev_p = p;
}

void discrete_integral_reset(discrete_integral *i) {
    i->prev_p.t = -1;
    i->prev_p.v = 0;
    i->sum = 0;
}

void pid_setup(pid_ctrl * controller, const pid_gains K, read_sensor feedback_sensor,
               read_sensor feedforward_sensor, apply_input plant, uint16_t time_between_ticks_ms,
               const pid_data_t windup_lb, const pid_data_t windup_ub, uint derivative_filter_span_ms){
    controller->K = K;
    controller->sensor = feedback_sensor;
    controller->sensor_feedforward = feedforward_sensor;
    controller->plant = plant;
    controller->min_time_between_ticks_ms = time_between_ticks_ms;
    controller->setpoint = 0;
    controller->_next_tick_time = get_absolute_time();

    discrete_integral_setup(&(controller->err_sum), windup_lb, windup_ub);
    discrete_derivative_setup(&(controller->err_slope), derivative_filter_span_ms, time_between_ticks_ms/2);
}

float pid_tick(pid_ctrl * controller){
    if(absolute_time_diff_us(get_absolute_time(), controller->_next_tick_time) < 0){
        controller->_next_tick_time = delayed_by_ms(get_absolute_time(), controller->min_time_between_ticks_ms);
        const datapoint new_reading = {.t = ms_since_boot(), .v = controller->sensor()};
        const datapoint new_err = {.t = new_reading.t, .v = controller->setpoint - new_reading.v};

        discrete_integral_add_datapoint(&(controller->err_sum), new_err);
        discrete_derivative_add_datapoint(&(controller->err_slope), new_reading);
        
        const pid_data_t e_sum   = (controller->K.i == 0 ? 0 : discrete_integral_read(&(controller->err_sum)));
        const pid_data_t e_slope = (controller->K.d == 0 ? 0 : discrete_derivative_read(&(controller->err_slope)));
        const pid_data_t ff = (controller->sensor_feedforward != NULL ? controller->sensor_feedforward() : 0);

        float input = (controller->K.p)*new_err.v + (controller->K.i)*e_sum + (controller->K.d)*e_slope + (controller->K.f)*ff;

        if(controller->plant != NULL) controller->plant(input);
        return input;
    }
    return 0;
}

bool pid_at_setpoint(pid_ctrl * controller, pid_data_t tol){
    const pid_data_t err = controller->sensor() - controller->setpoint;
    return (err >= -tol && err <= tol);
}

void pid_reset(pid_ctrl * controller){
    discrete_derivative_reset(&(controller->err_slope));
    discrete_integral_reset(&(controller->err_sum));
}

void pid_deinit(pid_ctrl * controller){
    discrete_derivative_deinit(&(controller->err_slope));
}