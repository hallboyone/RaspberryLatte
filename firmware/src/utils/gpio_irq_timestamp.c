/** @ingroup gpio_irq_timestamp
 * @{
 * 
 * \file gpio_irq_timestamp.c
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief GPIO IRQ Timestamp source
 * \version 0.1
 * \date 2023-01-23 
 */

#include "utils/gpio_irq_timestamp.h"
#include "utils/gpio_multi_callback.h"

static absolute_time_t _timestamps [32];
static uint32_t _events [32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static void gpio_irq_timestamp_cb(uint gpio, uint32_t event, void* data){
    _timestamps[gpio] = get_absolute_time();
}

int gpio_irq_timestamp_setup(uint8_t gpio, uint32_t events){
    if(gpio>=32) return PICO_ERROR_INVALID_ARG;
    if(!(events & (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE))) return PICO_ERROR_INVALID_ARG;

    // If indicated events are already watched, no reason to re-add it.
    if(events == (events & _events[gpio])) return 0;

    // Save events and setup new ones
    _events[gpio] = _events[gpio] | events;
    gpio_multi_callback_attach(gpio, events, true, &gpio_irq_timestamp_cb, NULL);
    
    return PICO_ERROR_NONE;
}

absolute_time_t gpio_irq_timestamp_read(uint8_t gpio){
    if(gpio>=32) return nil_time;
    return get_absolute_time();
}

int64_t gpio_irq_timestamp_read_duration_us(uint8_t gpio){
    if(gpio>=32) return PICO_ERROR_INVALID_ARG;
    return absolute_time_diff_us(_timestamps[gpio], get_absolute_time());
}

int64_t gpio_irq_timestamp_read_duration_ms(uint8_t gpio){
    return gpio_irq_timestamp_read_duration_us(gpio)/1000;
}

/** @} */