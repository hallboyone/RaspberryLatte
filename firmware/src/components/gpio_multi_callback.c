/**
 * \ingroup gpio_multi_callback
 * 
 * \file gpio_multi_callback.c
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief GPIO Multi-Callback source
 * \version 0.1
 * \date 2022-11-28
*/

#include "gpio_multi_callback.h"

static bool _irq_dispatch_setup = false;

static gpio_multi_callback_t _callbacks [32] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                                NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                                NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                                NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

/** \brief Pointer to callback data. */
static void* _callback_data [32] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

static uint32_t _callback_events [32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static void _gpio_multi_callback_irq_dispatch(uint8_t gpio, uint32_t event){
    assert(_callbacks[gpio] != NULL);
    _callbacks[gpio](gpio, event, _callback_data[gpio]);
}  

int gpio_multi_callback_attach(uint8_t gpio, uint32_t event_mask, bool enabled, gpio_multi_callback_t cb, void * data){
    assert(gpio < 32);
    assert(_callbacks[gpio] == NULL);
    assert(cb != NULL);

    // Add custom callback to list.
    _callbacks[gpio] = cb;
    _callback_data[gpio] = data;
    _callback_events[gpio] = event_mask;

    // Setup local dispatch if not already done 
    if(!_irq_dispatch_setup){
        gpio_set_irq_enabled_with_callback(gpio, event_mask, true, &_gpio_multi_callback_irq_dispatch);
        _irq_dispatch_setup = true;
    }

    // Setup GPIO as interrupt
    gpio_set_irq_enabled(gpio, event_mask, enabled);

    return PICO_ERROR_NONE;
}

int gpio_multi_callback_enabled(uint8_t gpio, bool enable){
    assert(gpio < 32);
    assert(_callbacks[gpio] == NULL);

    gpio_set_irq_enabled(gpio, _callback_events[gpio], enable);

    return PICO_ERROR_NONE;
}

int gpio_multi_callback_clear(uint8_t gpio){
    assert(gpio < 32);
    gpio_set_irq_enabled(gpio, 0, false);
    _callbacks[gpio] = NULL;
    _callback_data[gpio] = NULL;
    _callback_events[gpio] = 0;
}