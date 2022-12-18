/**
 * \file flow_meter.c
 * \ingroup flow_meter
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Flow Meter driver source
 * \version 0.1
 * \date 2022-12-10
 */

#include "flow_meter.h"

#include "gpio_multi_callback.h"

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
    datapoint dp = {.t = sec_since_boot(), .v = fm->pulse_count};
    discrete_derivative_add_point(&(fm->flow_rate), dp);
}

int flow_meter_setup(flow_meter * fm, uint8_t pin_num, float conversion_factor){
    if(pin_num >= 32) return PICO_ERROR_INVALID_ARG;

    gpio_set_dir(pin_num, false);
    gpio_set_pulls(pin_num, false, true);

    fm->pin = pin_num;
    fm->conversion_factor = conversion_factor;
    fm->pulse_count = 0;
    
    discrete_derivative_init(&(fm->flow_rate), 250);

    return gpio_multi_callback_attach(pin_num, GPIO_IRQ_EDGE_FALL, true, &_flow_meter_callback, fm);
}