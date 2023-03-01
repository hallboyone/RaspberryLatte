/**
 * \file flow_meter.c
 * \ingroup flow_meter
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Flow Meter driver source
 * \version 0.1
 * \date 2022-12-10
 */

#include "drivers/flow_meter.h"

#include "utils/gpio_multi_callback.h"

/**
 * \brief Simple callback that increments the corresponding flow_meter's pulse count by 1 and
 * adds the new point to the discrete_derivative
 * 
 * \param gpio GPIO number triggering callback
 * \param event Event triggering callback
 * \param data pointer to flow_meter associated with GPIO number
 */
static void _flow_meter_callback(uint gpio, uint32_t event, void* data){
    flow_meter * fm = (flow_meter*)data;
    fm->pulse_count += 1;
    discrete_derivative_add_value(&(fm->flow_rate), fm->pulse_count);
}

int flow_meter_setup(flow_meter * fm, uint8_t pin_num, float conversion_factor){
    if(pin_num >= 32) return PICO_ERROR_INVALID_ARG;

    gpio_set_dir(pin_num, false);
    gpio_set_pulls(pin_num, false, true);

    fm->pin = pin_num;
    fm->conversion_factor = conversion_factor;
    fm->pulse_count = 0;
    
    discrete_derivative_setup(&(fm->flow_rate), 1000, 10);

    // Every time the flow meter pin falls, trigger callback
    return gpio_multi_callback_attach(pin_num, GPIO_IRQ_EDGE_FALL, true, &_flow_meter_callback, fm);
}

float flow_meter_volume(flow_meter * fm){
    return fm->pulse_count * fm->conversion_factor;
}

float flow_meter_rate(flow_meter * fm){
    discrete_derivative_add_value(&(fm->flow_rate), fm->pulse_count);
    return discrete_derivative_read(&(fm->flow_rate)) * fm->conversion_factor * 1000.;
}

void flow_meter_zero(flow_meter * fm){
    fm->pulse_count = 0;
    discrete_derivative_reset(&(fm->flow_rate));
}