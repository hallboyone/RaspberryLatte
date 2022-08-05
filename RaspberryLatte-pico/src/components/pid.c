#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "pid.h"

#define _WINDUP_LOWER_BOUND_MIN -10000
#define _WINDUP_UPPER_BOUND_MAX 10000

/**
 * \brief Helper function returning the microseconds since booting
 */
static uint64_t us_since_boot(){
    return to_us_since_boot(get_absolute_time());
}

void discrete_derivative_init(discrete_derivative *d, uint filter_span_ms) {
    d->filter_span_ms = filter_span_ms;
    d->_buf_len = 10;
    d->_num_el = 0;
    d->_data = malloc((d->_buf_len) * sizeof(datapoint));
}

void discrete_derivative_deinit(discrete_derivative *d) {
    d->filter_span_ms = 0;
    d->_buf_len = 0;
    d->_num_el = 0;
    free(d->_data);
}

float discrete_derivative_read(discrete_derivative *d) {
    if (d->_num_el < 2) {
        return 0;
    }
    float v_sum = 0;
    uint64_t t_sum = 0;
    for (uint16_t i = 0; i < d->_num_el; i++) {
        v_sum += d->_data[i].v;
        t_sum += d->_data[i].t;
    }

    const float v_avg = v_sum / (float)d->_num_el;
    const float t_avg = t_sum / (float)d->_num_el;

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
 * d->filter_span_ms millaseconds behind cur_t. If all points are outside of
 * this range, the most recent point is always kept.
 */
void _discrete_derivative_remove_old_points(discrete_derivative *d, uint64_t cur_t) {
    uint16_t keep_from_idx = 0;
    for (keep_from_idx; keep_from_idx < d->_num_el - 1; keep_from_idx++) {
        // Find first point within the time range from cur_t
        if (cur_t - d->_data[keep_from_idx].t <= d->filter_span_ms) {
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

float discrete_derivative_add_point(discrete_derivative *d, datapoint p) {
    _discrete_derivative_remove_old_points(d, p.t);
    if (d->_num_el == d->_buf_len) {
        _discrete_derivative_expand_buf(d);
    }
    d->_data[d->_num_el] = p;
    d->_num_el += 1;
    return discrete_derivative_read(d);
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
    return i->sum; 
}

float discrete_integral_add_point(discrete_integral *i, datapoint p) {
    if (i->prev_p.t == 0) {
        i->prev_p = p;
        return 0;
    } else {
        i->sum += ((p.v + i->prev_p.v) / 2.0) * (p.t - i->prev_p.t);
        i->sum = (i->sum < i->lower_bound ? i->lower_bound : i->sum);
        i->sum = (i->sum > i->upper_bound ? i->upper_bound : i->sum);
        i->prev_p = p;
        return i->sum;
    }
}

void discrete_integral_reset(discrete_integral *i) {
    datapoint init_p = {.t = 0, .v = 0};
    i->prev_p = init_p;
    i->sum = 0;
}

void pid_init(pid_ctrl * controller, const float windup_lb, const float windup_ub, uint derivative_filter_span_ms){
    discrete_integral_init(&(controller->err_sum), windup_lb*1000000, windup_ub*1000000);
    discrete_derivative_init(&(controller->err_slope), derivative_filter_span_ms);
}

float pid_tick(pid_ctrl * controller){
    datapoint new_reading = {.t = us_since_boot(), .v = controller->sensor()};
    datapoint new_err = {.t = new_reading.t, .v = controller->setpoint - new_reading.v};

    // If Ki (Kd) non-zero, compute the error sum (slope). Convert from us to s.
    float e_sum   = (controller->K.i == 0 ? 0 : discrete_integral_add_point(&(controller->err_sum), new_err)/1000000.);
    float e_slope = (controller->K.d == 0 ? 0 : discrete_derivative_add_point(&(controller->err_slope), new_err)*1000000.);

    float input = (controller->K.p)*new_err.v + (controller->K.i)*e_sum + (controller->K.d)*e_slope;

    printf("Temp: %0.2fC, Error: %0.2fC, Error Sum: %0.4fC*s, Error Slope: %0.4fC/s, Input: %0.4f\n",
            new_reading.v, new_err.v, discrete_integral_read(&(controller->err_sum))/1000000., 
            discrete_derivative_read(&(controller->err_slope))*1000000., input);

    controller->plant(input);
    return 0;
}

void pid_reset(pid_ctrl * controller){
    discrete_derivative_reset(&(controller->err_slope));
    discrete_integral_reset(&(controller->err_sum));
}

void pid_deinit(pid_ctrl * controller){
    discrete_derivative_deinit(&(controller->err_slope));
}