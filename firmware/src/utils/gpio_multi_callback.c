/**
 * \ingroup gpio_multi_callback
 * 
 * \file gpio_multi_callback.c
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief GPIO Multi-Callback source
 * \version 0.1
 * \date 2022-11-28
*/

#include "utils/gpio_multi_callback.h"

static bool _irq_dispatch_setup = false;

typedef struct {
    gpio_multi_callback_t fun;
    uint32_t events;
    void * data;
} gpio_multi_callback_config_t;

static gpio_multi_callback_config_t _callbacks [32] = {
    {.fun = NULL, .events = 0, .data = NULL}, {.fun = NULL, .events = 0, .data = NULL},
    {.fun = NULL, .events = 0, .data = NULL}, {.fun = NULL, .events = 0, .data = NULL},
    {.fun = NULL, .events = 0, .data = NULL}, {.fun = NULL, .events = 0, .data = NULL},
    {.fun = NULL, .events = 0, .data = NULL}, {.fun = NULL, .events = 0, .data = NULL},
    {.fun = NULL, .events = 0, .data = NULL}, {.fun = NULL, .events = 0, .data = NULL},
    {.fun = NULL, .events = 0, .data = NULL}, {.fun = NULL, .events = 0, .data = NULL},
    {.fun = NULL, .events = 0, .data = NULL}, {.fun = NULL, .events = 0, .data = NULL},
    {.fun = NULL, .events = 0, .data = NULL}, {.fun = NULL, .events = 0, .data = NULL},
    {.fun = NULL, .events = 0, .data = NULL}, {.fun = NULL, .events = 0, .data = NULL},
    {.fun = NULL, .events = 0, .data = NULL}, {.fun = NULL, .events = 0, .data = NULL},
    {.fun = NULL, .events = 0, .data = NULL}, {.fun = NULL, .events = 0, .data = NULL},
    {.fun = NULL, .events = 0, .data = NULL}, {.fun = NULL, .events = 0, .data = NULL},
    {.fun = NULL, .events = 0, .data = NULL}, {.fun = NULL, .events = 0, .data = NULL},
    {.fun = NULL, .events = 0, .data = NULL}, {.fun = NULL, .events = 0, .data = NULL},
    {.fun = NULL, .events = 0, .data = NULL}, {.fun = NULL, .events = 0, .data = NULL},
    {.fun = NULL, .events = 0, .data = NULL}, {.fun = NULL, .events = 0, .data = NULL}};

/**
 * \brief Centralized callback that dispatches calls to the correct custom callback
 * 
 * \param gpio The GPIO pin that generated the interrupt
 * \param event The GPIO even the generated the interrupt
 */
static void _gpio_multi_callback_irq_dispatch(uint gpio, uint32_t event){
    // Call appropriate callback if set and events match.
    const gpio_multi_callback_config_t cb = _callbacks[gpio];
    if(cb.fun != NULL && (event & cb.events)) {
        cb.fun(gpio, event, cb.data);
    }
}  

int gpio_multi_callback_attach(uint8_t gpio, uint32_t event_mask, bool enabled, gpio_multi_callback_t cb, void * data){
    assert(gpio < 32);
    assert(_callbacks[gpio].fun == NULL);
    assert(cb != NULL);

    // Add custom callback to list.
    _callbacks[gpio].fun = cb;
    _callbacks[gpio].data = data;
    _callbacks[gpio].events = event_mask;

    // Setup local dispatch if not already done 
    if(!_irq_dispatch_setup){
        gpio_set_irq_enabled_with_callback(gpio, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &_gpio_multi_callback_irq_dispatch);
        _irq_dispatch_setup = true;
    }

    // Setup GPIO as interrupt
    gpio_set_irq_enabled(gpio, event_mask, enabled);

    return PICO_ERROR_NONE;
}

int gpio_multi_callback_enabled(uint8_t gpio, bool enable){
    assert(gpio < 32);
    assert(_callbacks[gpio].fun == NULL);

    gpio_set_irq_enabled(gpio, _callbacks[gpio].events, enable);

    return PICO_ERROR_NONE;
}

int gpio_multi_callback_clear(uint8_t gpio){
    assert(gpio < 32);
    gpio_set_irq_enabled(gpio, 0, false);
    _callbacks[gpio].fun = NULL;
    _callbacks[gpio].data = NULL;
    _callbacks[gpio].events = 0;
}