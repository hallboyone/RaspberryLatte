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

/** \brief Flag indicating if IRQ dispatch has been setup. */
static bool _irq_dispatch_setup = false;

/** \brief The callback function, triggering events, and optional data for a multi-callback. */
typedef struct {
    gpio_multi_callback_t fun; /**<\brief The callback function. */
    uint32_t events;           /**<\brief Event flags indicating what will trigger callback (see Pico SDK for more info).*/
    void * data;               /**<\brief Optional pointer to data that will be passed to callback. */
} gpio_multi_callback_config_t;

/** \brief Callback array for a single GPIO pin.
 * Each contains a pointer to a callback collection and the number of callbacks.
*/
typedef struct {
    gpio_multi_callback_config_t * callbacks; /**<\brief Array of configured callbacks. */
    uint8_t num_cb;                           /**<\brief The number of configured callbacks. */
} gpio_multi_callback_array_t;

/** \brief Array of callback arrays. One for each GPIO pin. */
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