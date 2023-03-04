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

#include "utils/gpio_multi_callback.h"

/** \brief Structure managing a single flowmeter. */
typedef struct flow_meter_s {
    uint8_t pin;                   /**< \brief The GPIO attached to the flow meter's signal. */
    uint conversion_factor;        /**< \brief Factor converting pulse counts to volume. */
    uint pulse_count;              /**< \brief Number of pulses since last zero. */
    discrete_derivative flow_rate; /**< \brief Derivative structure for tracking the flow rate in pulse/ms. */
} flow_meter_;

/**
 * \brief Simple callback that increments the corresponding flow_meter's pulse count by 1 and
 * adds the new point to the discrete_derivative
 * 
 * \param gpio GPIO number triggering callback
 * \param event Event triggering callback
 * \param data pointer to flow_meter associated with GPIO number
 */
static void _flow_meter_callback(uint gpio, uint32_t event, void* data){
    flow_meter fm = (flow_meter)data;
    fm->pulse_count += 1;
    discrete_derivative_add_value(fm->flow_rate, fm->pulse_count);
}

flow_meter flow_meter_setup(uint8_t pin_num, uint16_t conversion_factor, 
                            uint16_t filter_span_ms, uint16_t sample_dwell_time_ms){
    flow_meter fm = malloc(sizeof(flow_meter_));

    if(pin_num >= 32) return PICO_ERROR_INVALID_ARG;

    gpio_set_dir(pin_num, false);
    gpio_set_pulls(pin_num, false, true);

    fm->pin = pin_num;
    fm->conversion_factor = conversion_factor;
    fm->pulse_count = 0;
    
    fm->flow_rate = discrete_derivative_setup(filter_span_ms, sample_dwell_time_ms);

    // Every time the flow meter pin falls, trigger callback
    return gpio_multi_callback_attach(pin_num, GPIO_IRQ_EDGE_FALL, true, &_flow_meter_callback, fm);
}

uint flow_meter_volume(flow_meter fm){
    return fm->pulse_count * fm->conversion_factor;
}

float flow_meter_rate(flow_meter fm){
    discrete_derivative_add_value(fm->flow_rate, fm->pulse_count);
    // scale by 1000 to convert 1/ms to 1/s.
    return discrete_derivative_read(fm->flow_rate) * fm->conversion_factor * 1000;
}

void flow_meter_zero(flow_meter fm){
    fm->pulse_count = 0;
    discrete_derivative_reset(fm->flow_rate);
}