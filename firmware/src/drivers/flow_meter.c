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
    uint8_t id;                    /**< \brief The id of the flow_meter in array. */
    float conversion_factor;       /**< \brief Factor converting pulse counts to volume. */
    uint pulse_count;              /**< \brief Number of pulses since last zero. */
    discrete_derivative flow_rate; /**< \brief Derivative structure for tracking the flow rate in pulse/ms. */
} flow_meter_s;

/** Array of all configurable flow meter structures. */
static flow_meter_s _fm_arr [FLOW_METER_MAX_OCCURRENCES];

/** Number of flow meters that have been configured. */
static uint8_t _num_flow_meters = 0;

/**
 * \brief Simple callback that increments the corresponding flow_meter's pulse count by 1 and
 * adds the new point to the discrete_derivative
 * 
 * \param gpio GPIO number triggering callback
 * \param event Event triggering callback
 * \param data pointer to flow_meter associated with GPIO number
 */
static void _flow_meter_callback(uint gpio, uint32_t event, void* data){
    UNUSED_PARAMETER(gpio);
    UNUSED_PARAMETER(event);
    flow_meter fm = *(uint8_t*)data;
    _fm_arr[fm].pulse_count += 1;
    discrete_derivative_add_value(_fm_arr[fm].flow_rate, _fm_arr[fm].pulse_count);
}

flow_meter flow_meter_setup(uint8_t pin_num, float conversion_factor, 
                            uint16_t filter_span_ms, uint16_t sample_dwell_time_ms){
    
    if(pin_num >= 32) return -1;
    if(_num_flow_meters >= FLOW_METER_MAX_OCCURRENCES) return -1;

    gpio_set_dir(pin_num, false);
    gpio_set_pulls(pin_num, false, true);

    _fm_arr[_num_flow_meters].id = _num_flow_meters;
    _fm_arr[_num_flow_meters].conversion_factor = conversion_factor;
    _fm_arr[_num_flow_meters].pulse_count = 0;
    
    _fm_arr[_num_flow_meters].flow_rate = discrete_derivative_setup(filter_span_ms, sample_dwell_time_ms);

    // Every time the flow meter pin falls, trigger callback
    if(PICO_ERROR_NONE != gpio_multi_callback_attach(pin_num, 
                                                     GPIO_IRQ_EDGE_FALL, 
                                                     true, 
                                                     &_flow_meter_callback, 
                                                     &_fm_arr[_num_flow_meters].id)){
        return -1;
    }
    _num_flow_meters += 1;
    return _fm_arr[_num_flow_meters-1].id;
}

float flow_meter_volume(flow_meter fm){
    return _fm_arr[fm].pulse_count * _fm_arr[fm].conversion_factor;
}

float flow_meter_rate(flow_meter fm){
    // Values are only added when pulse is read. Add value here to go to zero when no pulses happen.
    discrete_derivative_add_value(_fm_arr[fm].flow_rate, _fm_arr[fm].pulse_count);
    // discrete_derivatives are in units/ms. Scale by 1000 to get units/s
    return 1000.0 * discrete_derivative_read(_fm_arr[fm].flow_rate) * _fm_arr[fm].conversion_factor;
}

void flow_meter_zero(flow_meter fm){
    _fm_arr[fm].pulse_count = 0;
    discrete_derivative_reset(_fm_arr[fm].flow_rate);
}

void flow_meter_deinit(flow_meter fm){
    discrete_derivative_deinit(_fm_arr[fm].flow_rate);
}