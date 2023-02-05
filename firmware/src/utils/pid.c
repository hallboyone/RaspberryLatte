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

/**
 * \brief Helper function returning the microseconds since booting
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
    d->_buf_len = 10;
    d->_num_el = 0;
    d->_data = malloc((d->_buf_len) * sizeof(datapoint));
}

/**
 * \brief Release internal memory and reset object.
 * 
 * \param d Pointer to discrete_derivative that will be destroyed
 */
void discrete_derivative_deinit(discrete_derivative *d) {
    d->filter_span_ms = 0;
    d->_buf_len = 0;
    d->_num_el = 0;
    free(d->_data);
}

/**
 * \brief Computes the linear slope of the current datapoints.
 * 
 * \param d Pointer to discrete_derivative that will be read
 * 
 * \returns The slope of the previous datapoints within the filter_span of d. If only
 * 0 or 1 point, returns 0.
 */
float discrete_derivative_read(discrete_derivative *d) {
    if (d->_num_el < 2) {
        return 0;
    }
    float v_avg = 0;
    float t_avg = 0;
    for (uint16_t i = 0; i < d->_num_el; i++) {
        v_avg += d->_data[i].v;
        t_avg += d->_data[i].t;
    }
    v_avg /= (float)d->_num_el;
    t_avg /= (float)d->_num_el;

    float num = 0;
    float dem = 0;
    for (uint16_t i = 0; i < d->_num_el; i++) {
        num = num + (d->_data[i].t - t_avg) * (d->_data[i].v - v_avg);
        dem = dem + ((d->_data[i].t - t_avg) * (d->_data[i].t - t_avg));
    }
    return num / dem;
}

/**
 * \brief Removes all points in d's _data buf with timestamps more than
 * d->filter_span_ms milliseconds behind cur_t. If all points are outside of
 * this range, the most recent point is always kept.
 */
void _discrete_derivative_remove_old_points(discrete_derivative *d, float cur_t) {
    uint16_t keep_from_idx = 0;
    for (keep_from_idx; keep_from_idx < d->_num_el - 1; keep_from_idx++) {
        // Find first point within the time range from cur_t
        if ((cur_t - d->_data[keep_from_idx].t)*1000 <= d->filter_span_ms) {
            break;
        }
    }
    // If this is not the first point in array
    if (keep_from_idx > 0) {
        // Shift first point to start of array
        d->_num_el -= keep_from_idx;
        memcpy(d->_data, &(d->_data[keep_from_idx]), (d->_num_el) * sizeof(datapoint));
    }
}

/**
 * \brief Doubles the size of the _data buffer in d.
 */
void _discrete_derivative_expand_buf(discrete_derivative *d) {
    d->_buf_len *= 2;
    datapoint *new_buf = malloc(sizeof(datapoint) * d->_buf_len);
    memcpy(new_buf, d->_data, (d->_num_el) * sizeof(datapoint));
    free(d->_data);
    d->_data = new_buf;
}

void discrete_derivative_add_point(discrete_derivative *d, datapoint p) {
    _discrete_derivative_remove_old_points(d, p.t);
    if (d->_num_el == d->_buf_len) {
        _discrete_derivative_expand_buf(d);
    }
    d->_data[d->_num_el] = p;
    d->_num_el += 1;
}

void discrete_derivative_reset(discrete_derivative *d) { 
    d->_num_el = 0; 
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

void pid_reset(pid_ctrl * controller){
    discrete_derivative_reset(&(controller->err_slope));
    discrete_integral_reset(&(controller->err_sum));
}

void pid_deinit(pid_ctrl * controller){
    discrete_derivative_deinit(&(controller->err_slope));
}