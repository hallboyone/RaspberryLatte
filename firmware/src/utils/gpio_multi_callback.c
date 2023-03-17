/**
 * \ingroup gpio_multi_callback
 * 
 * \file gpio_multi_callback.c
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief GPIO Multi-Callback source
 * \version 0.2
 * \date 2023-01-24
*/

#include "utils/gpio_multi_callback.h"

#include <stdlib.h>
#include <string.h>

static bool _irq_dispatch_setup = false;

typedef struct {
    gpio_multi_callback_t fun;
    uint32_t events;
    void * data;
} gpio_multi_callback_config_t;

typedef struct {
    gpio_multi_callback_config_t * callbacks;
    uint8_t num_cb;
} gpio_multi_callback_array_t;

static gpio_multi_callback_array_t _callbacks [32] = {
    {.callbacks = NULL, .num_cb = 0}, {.callbacks = NULL, .num_cb = 0},
    {.callbacks = NULL, .num_cb = 0}, {.callbacks = NULL, .num_cb = 0},
    {.callbacks = NULL, .num_cb = 0}, {.callbacks = NULL, .num_cb = 0},
    {.callbacks = NULL, .num_cb = 0}, {.callbacks = NULL, .num_cb = 0},
    {.callbacks = NULL, .num_cb = 0}, {.callbacks = NULL, .num_cb = 0},
    {.callbacks = NULL, .num_cb = 0}, {.callbacks = NULL, .num_cb = 0},
    {.callbacks = NULL, .num_cb = 0}, {.callbacks = NULL, .num_cb = 0},
    {.callbacks = NULL, .num_cb = 0}, {.callbacks = NULL, .num_cb = 0},
    {.callbacks = NULL, .num_cb = 0}, {.callbacks = NULL, .num_cb = 0},
    {.callbacks = NULL, .num_cb = 0}, {.callbacks = NULL, .num_cb = 0},
    {.callbacks = NULL, .num_cb = 0}, {.callbacks = NULL, .num_cb = 0},
    {.callbacks = NULL, .num_cb = 0}, {.callbacks = NULL, .num_cb = 0},
    {.callbacks = NULL, .num_cb = 0}, {.callbacks = NULL, .num_cb = 0},
    {.callbacks = NULL, .num_cb = 0}, {.callbacks = NULL, .num_cb = 0},
    {.callbacks = NULL, .num_cb = 0}, {.callbacks = NULL, .num_cb = 0},
    {.callbacks = NULL, .num_cb = 0}, {.callbacks = NULL, .num_cb = 0}};

/**
 * \brief Centralized callback that dispatches calls to the correct custom callback
 * 
 * \param gpio The GPIO pin that generated the interrupt
 * \param event The GPIO even the generated the interrupt
 */
static void _gpio_multi_callback_irq_dispatch(uint gpio, uint32_t event){
    // Call all callbacks with matching events.
    const gpio_multi_callback_array_t cb = _callbacks[gpio];
    for(uint8_t i = 0; i < cb.num_cb; i++){
        if(event & cb.callbacks[i].events) {
            cb.callbacks[i].fun(gpio, event, cb.callbacks[i].data);
        }
    }
}  

int gpio_multi_callback_attach(uint8_t gpio, uint32_t event_mask, bool enabled, gpio_multi_callback_t cb, void * data){
    assert(gpio < 32);
    assert(cb != NULL);

    const uint8_t idx = _callbacks[gpio].num_cb;

    // Just adding one to callback array. Computationally inefficient but uses less memory.
    gpio_multi_callback_config_t * new_arr = malloc((idx+1) * sizeof(gpio_multi_callback_config_t));
    memcpy(new_arr, _callbacks[gpio].callbacks, (idx+1) * sizeof(gpio_multi_callback_config_t));
    free(_callbacks[gpio].callbacks);
    _callbacks[gpio].callbacks = new_arr;
    _callbacks[gpio].num_cb += 1;
    
    // Add custom callback to list.
    _callbacks[gpio].callbacks[idx].fun = cb;
    _callbacks[gpio].callbacks[idx].data = data;
    _callbacks[gpio].callbacks[idx].events = event_mask;

    // Setup local dispatch if not already done 
    if(!_irq_dispatch_setup){
        gpio_set_irq_enabled_with_callback(gpio, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &_gpio_multi_callback_irq_dispatch);
        _irq_dispatch_setup = true;
    }

    // Setup GPIO as interrupt
    gpio_set_irq_enabled(gpio, event_mask, enabled);
    
    return PICO_ERROR_NONE;
}

int gpio_multi_callback_enabled(uint8_t gpio, uint32_t event_mask, bool enable){
    assert(gpio < 32);

    gpio_set_irq_enabled(gpio, event_mask, enable);

    return PICO_ERROR_NONE;
}

int gpio_multi_callback_clear(uint8_t gpio){
    assert(gpio < 32);
    gpio_set_irq_enabled(gpio, 0, false);
    free(_callbacks[gpio].callbacks);
    _callbacks[gpio].callbacks = NULL;
    _callbacks[gpio].num_cb = 0;
    return PICO_ERROR_NONE;
}