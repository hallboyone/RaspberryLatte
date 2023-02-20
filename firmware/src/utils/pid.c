/**
 * \ingroup pid
 * 
 * \file pid.c
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief PID Library source
 * \version 0.1
 * \date 2022-08-16
*/

#include "utils/pid.h"

#include <stdlib.h>
#include <string.h>

struct _datapoint_ext_{
    float v;
    float t;
    float tt;
    float vt;
};

/**
 * \brief Helper function returning the seconds since booting
 */
float sec_since_boot(){
    return to_us_since_boot(get_absolute_time())/1000000.;
}

/**
 * \brief Clear the internal fields of struct d and initalize.
 * 
 * \param d Pointer to discrete_derivative that will be initalized
 * \param filter_span_ms Slope is computed over all datapoints taken within the last filter_span_ms
 */
void discrete_derivative_init(discrete_derivative *d, uint filter_span_ms) {
    d->filter_span_ms = filter_span_ms;
    d->_buf_len = 16;
    d->_num_el = 0;
    d->_data = malloc((d->_buf_len) * sizeof(_datapoint_ext));
}

/**
 * \brief Release internal memory and reset object.
 * 
 * \param d Pointer to discrete_derivative that will be destroyed
 */
void discrete_derivative_deinit(discrete_derivative *d) {
    discrete_derivative_reset(d);
    d->filter_span_ms = 0;
    free(d->_data);
}

/**
 * \brief Computes the linear slope of the current datapoints.
 * 
 * \param d Pointer to discrete_derivative that will be read
 * 
 * \returns The slope of the previous datapoints within the filter_span of d. If less 
 * than 2 datapoints, returns 0.
 */
float discrete_derivative_read(discrete_derivative *d) {
    if (d->_num_el < 2) return 0;

    // Compute and return
    const float v_avg = d->_sum_v/d->_num_el;
    const float t_avg = d->_sum_t/d->_num_el;
    return (d->_sum_vt - d->_sum_t*v_avg)/(d->_sum_tt - d->_sum_t*t_avg);
}

/**
 * \brief Removes all points in d's _data buf with timestamps more than
 * d->filter_span_ms milliseconds behind cur_t. 
 * 
 * If all points are outside of this range, the most recent point is always kept.
 * 
 * \param d Pointer to a \ref discrete_derivative that will be cleaned
 * \param cur_t The current timestamp as seconds-since-boot
 */
static void _discrete_derivative_remove_old_points(discrete_derivative *d, float cur_t) {
    while((cur_t - d->_data[d->_start_idx].t)*1000 > d->filter_span_ms && d->_num_el > 1){
        d->_sum_t -= d->_data[d->_start_idx].t;
        d->_sum_v -= d->_data[d->_start_idx].v;
        d->_sum_tt -= d->_data[d->_start_idx].tt;
        d->_sum_vt -= d->_data[d->_start_idx].vt;
        d->_start_idx = (d->_start_idx+1) % d->_buf_len; // Increment starting index and wrap arround buf
        d->_num_el -= 1;
    }
}

/** \brief Doubles the size of the _data buffer in d. */
static void _discrete_derivative_expand_buf(discrete_derivative *d) {
    // Get current layout of data in buffer
    const bool wraps = (d->_start_idx + d->_num_el > d->_buf_len);
    const uint16_t len_from_0 = (wraps ? (d->_start_idx + d->_num_el) % d->_buf_len : 0);
    const uint16_t len_from_start = d->_num_el - len_from_0;
    
    // Double _buf_len and make new buffer
    d->_buf_len <<= 1;
    _datapoint_ext * new_buf = malloc(sizeof(_datapoint_ext) * d->_buf_len);
    
    // Copy the data into new buffer
    memcpy(new_buf, &(d->_data[d->_start_idx]), len_from_start * sizeof(_datapoint_ext));
    if(len_from_0 > 0){
        memcpy(&(new_buf[len_from_start]), d->_data, len_from_0 * sizeof(_datapoint_ext));
    }

    // Free mem and updat internal vars to new buffer
    free(d->_data);
    d->_data = new_buf;
    d->_start_idx = 0;
}

void discrete_derivative_add_point(discrete_derivative *d, datapoint p) {
    // Remove old points and expand buffer if needed
    _discrete_derivative_remove_old_points(d, p.t);
    if (d->_num_el == d->_buf_len) _discrete_derivative_expand_buf(d);

    // Add new point to buffer
    const uint16_t high_idx = (d->_start_idx + d->_num_el) % d->_buf_len;
    d->_data[high_idx].v = p.v;
    d->_data[high_idx].t = p.t;
    d->_data[high_idx].tt = p.t*p.t;
    d->_data[high_idx].vt = p.t*p.v;

    // Update internal vars.
    d->_sum_t += d->_data[high_idx].t;
    d->_sum_v += d->_data[high_idx].v;
    d->_sum_tt += d->_data[high_idx].tt;
    d->_sum_vt += d->_data[high_idx].vt;
    d->_num_el += 1;
}

void discrete_derivative_reset(discrete_derivative *d) { 
    d->_start_idx = 0;
    d->_num_el = 0; 
    d->_sum_t = 0;
    d->_sum_v = 0;
    d->_sum_tt = 0;
    d->_sum_vt = 0;
}

void discrete_integral_init(discrete_integral *i, const float lower_bound,
                            const float upper_bound) {
    discrete_integral_reset(i);
    i->lower_bound = lower_bound;
    i->upper_bound = upper_bound;
}

float discrete_integral_read(discrete_integral *i) { 
    return (i->prev_p.t != 0) ? i->sum : 0; 
}

void discrete_integral_add_point(discrete_integral *i, datapoint p) {
    if (i->prev_p.t != 0) {
        i->sum += ((p.v + i->prev_p.v) / 2.0) * (p.t - i->prev_p.t);
        i->sum = (i->sum < i->lower_bound ? i->lower_bound : i->sum);
        i->sum = (i->sum > i->upper_bound ? i->upper_bound : i->sum);
    }
    i->prev_p = p;
}

void discrete_integral_reset(discrete_integral *i) {
    datapoint init_p = {.t = 0, .v = 0};
    i->prev_p = init_p;
    i->sum = 0;
}

void pid_setup(pid_ctrl * controller, const pid_gains K, read_sensor feedback_sensor,
               read_sensor feedforward_sensor, apply_input plant, uint16_t time_between_ticks_ms,
               const float windup_lb, const float windup_ub, uint derivative_filter_span_ms){
    controller->K = K;
    controller->sensor = feedback_sensor;
    controller->sensor_feedforward = feedforward_sensor;
    controller->plant = plant;
    controller->min_time_between_ticks_ms = time_between_ticks_ms;
    controller->setpoint = 0;
    controller->_next_tick_time = get_absolute_time();

    discrete_integral_init(&(controller->err_sum), windup_lb, windup_ub);
    discrete_derivative_init(&(controller->err_slope), derivative_filter_span_ms);
}

float pid_tick(pid_ctrl * controller){
    if(absolute_time_diff_us(get_absolute_time(), controller->_next_tick_time) < 0){
        controller->_next_tick_time = delayed_by_ms(get_absolute_time(), controller->min_time_between_ticks_ms);
        datapoint new_reading = {.t = sec_since_boot(), .v = controller->sensor()};
        datapoint new_err = {.t = new_reading.t, .v = controller->setpoint - new_reading.v};

        discrete_integral_add_point(&(controller->err_sum), new_err);
        discrete_derivative_add_point(&(controller->err_slope), new_err);
        
        float e_sum   = (controller->K.i == 0 ? 0 : discrete_integral_read(&(controller->err_sum)));
        float e_slope = (controller->K.d == 0 ? 0 : discrete_derivative_read(&(controller->err_slope)));
        float ff = (controller->sensor_feedforward != NULL ? controller->sensor_feedforward() : 0);
        float input = (controller->K.p)*new_err.v + (controller->K.i)*e_sum + (controller->K.d)*e_slope + (controller->K.f)*ff;

        if(controller->plant != NULL){
            controller->plant(input);
        }
        return input;
    }
    return 0;
}

/**
 * \brief Checks if the plant is at the target setpoint +/- a tolerance
 * 
 * \param controller Pointer to PID controller object to check.
 * \param tol The range around the setpoint that are considered good.
 * 
 * \return True if at setpoint. False otherwise.
 */
bool pid_at_setpoint(pid_ctrl * controller, float tol){
    const float err = controller->sensor() - controller->setpoint;
    return (err >= -tol && err <= tol);
}

void pid_reset(pid_ctrl * controller){
    discrete_derivative_reset(&(controller->err_slope));
    discrete_integral_reset(&(controller->err_sum));
}

void pid_deinit(pid_ctrl * controller){
    discrete_derivative_deinit(&(controller->err_slope));
}