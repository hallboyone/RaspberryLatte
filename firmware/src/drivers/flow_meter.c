/**
 * \file flow_meter.c
 * \ingroup flow_meter
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Flow Meter driver source
 * \version 0.1
 * \date 2022-12-10
 */

#include "drivers/flow_meter.h"

#include <stdlib.h>
#include <stdio.h>

#include "utils/gpio_multi_callback.h"
#include "utils/macros.h"

/** The maximum number of flow meters that library can configure. */
#define FLOW_METER_MAX_OCCURRENCES 1

/** \brief Structure managing a single flowmeter. */
typedef struct {
    double conversion_factor;/**< \brief Factor converting pulse counts to volume. */
    uint pulse_count;       /**< \brief Number of pulses since last zero. */
    int64_t t_delta_us;     /**< \brief The duration between the last to time stamps. */
    absolute_time_t t_prev; /**< \brief The timestamp of previous pulse. */
} flow_meter_s;

/** Array of all configurable flow meter structures. */
static flow_meter_s _fm_arr [FLOW_METER_MAX_OCCURRENCES];

/** Number of flow meters that have been configured. */
static uint8_t _num_flow_meters = 0;

/**
 * \brief Callback used to register flow meter pulse.
 * 
 * Increments the corresponding flow_meter's pulse count by 1 and records the current 
 * time and duration since last pulse.
 * 
 * \param gpio GPIO number triggering callback
 * \param event Event triggering callback
 * \param data pointer to flow_meter associated with GPIO number
 */
static void _flow_meter_callback(uint gpio, uint32_t event, void* data){
    UNUSED_PARAMETER(gpio);
    UNUSED_PARAMETER(event);
    flow_meter_s * fm = (flow_meter_s*)data;
    
    fm->pulse_count += 1;

    if(is_nil_time(fm->t_prev)){ // First pulse
        fm->t_prev = get_absolute_time();
    } else { // Record new pulse time and previous delta
        absolute_time_t new_pulse_time = get_absolute_time();
        fm->t_delta_us = absolute_time_diff_us(fm->t_prev, new_pulse_time);
        fm->t_prev = new_pulse_time;
    }
}

flow_meter flow_meter_setup(uint8_t pin_num, double conversion_factor){
    
    if(pin_num >= 32) return -1;
    if(_num_flow_meters >= FLOW_METER_MAX_OCCURRENCES) return -1;

    gpio_set_dir(pin_num, false);
    gpio_set_pulls(pin_num, false, true);

    _fm_arr[_num_flow_meters].conversion_factor = conversion_factor;
    flow_meter_zero(_num_flow_meters);

    // Every time the flow meter pin falls, trigger callback
    if(PICO_ERROR_NONE != gpio_multi_callback_attach(pin_num, 
                                                     GPIO_IRQ_EDGE_FALL, 
                                                     true, 
                                                     &_flow_meter_callback, 
                                                     &_fm_arr[_num_flow_meters])){
        return -1;
    }
    _num_flow_meters += 1;
    return _num_flow_meters-1;
}

double flow_meter_volume(flow_meter fm){
    return _fm_arr[fm].pulse_count * _fm_arr[fm].conversion_factor;
}

double flow_meter_rate(flow_meter fm){
    if(is_nil_time(_fm_arr[fm].t_prev)) return 0;

    const int64_t time_since_pulse = absolute_time_diff_us(_fm_arr[fm].t_prev, get_absolute_time());
    const double t_delta_s = (time_since_pulse < _fm_arr[fm].t_delta_us ? _fm_arr[fm].t_delta_us: time_since_pulse)/1000000.;
    const double slope = _fm_arr[fm].conversion_factor/t_delta_s;
    return (slope < 0.05 ? 0 : slope);
}

void flow_meter_zero(flow_meter fm){
    _fm_arr[fm].pulse_count = 0;
    _fm_arr[_num_flow_meters].t_prev = nil_time;
    _fm_arr[_num_flow_meters].t_delta_us = 0;
}