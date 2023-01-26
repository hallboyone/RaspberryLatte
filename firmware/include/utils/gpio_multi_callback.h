/**
 * \defgroup gpio_multi_callback GPIO Multi-Callback Library
 * \ingroup utils
 * 
 * \brief Library providing functionality for multiple GPIO callbacks.
 * 
 * Using the standard pico SDK, only a single GPIO callback can be defined. This library
 * allows for multiple custom callbacks to be assigned to each GPIO pin. 
 * 
 * \note This library should completely replace the standard GPIO callback routines in the 
 * pico SDK. If one component of your firmware uses this library, all components should use
 * it.
 * 
 * \{
 * 
 * \file gpio_multi_callback.h
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief GPIO Multi-Callback header
 * \version 0.2
 * \date 2023-01-24
*/
#ifndef GPIO_MULTI_CALLBACK
#define GPIO_MULTI_CALLBACK

#include "pico/stdlib.h"

typedef void (*gpio_multi_callback_t)(uint gpio, uint32_t event, void* data);

/**
 * \brief Attach a callback to a GPIO for specific events
 * 
 * \ref gpio must be a valid pin number and cannot have been setup. Furthermore, the
 * callback cannot be NULL. These conditions are checked with asserts. 
 * 
 * \param gpio The GPIO number to attach to
 * \param event_mask The events that will trigger interrupt
 * \param enabled Flag indicating if interrupt should be active
 * \param cb Pointer to callback function
 * \param data User data that will be passed to callback function
 * \return PICO_ERROR_NONE if no error. Errors will crash the program
 */
int gpio_multi_callback_attach(uint8_t gpio, uint32_t event_mask, bool enabled, gpio_multi_callback_t cb, void * data);

/**
 * \brief Enable or disable the interrupt for the given events
 * 
 * Equivelent to the \ref gpio_set_irq_enabled function in the pico SDK. This function
 * just asserts that the gpio had been previously setup.
 * 
 * \param gpio The GPIO number ot modify.
 * \param enable Flag indicating if the events should be enabled or disabled.
 * \return PICO_ERROR_NONE 
 */
int gpio_multi_callback_enabled(uint8_t gpio, uint32_t event_mask, bool enable);

/**
 * \brief Disable a GPIO callback and clear its associated fields
 * 
 * GPIO must have been previously setup. 
 * 
 * \param gpio GPIO to clear
 * \return PICO_ERROR_NONE 
 */
int gpio_multi_callback_clear(uint8_t gpio);

#endif

/** \} */