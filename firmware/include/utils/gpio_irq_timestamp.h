/** \defgroup gpio_irq_timestamp GPIO IRQ Timestamp Library
 * \ingroup utils
 * \brief Tracks the timestamps of IRQ events in configured GPIO pins
 * 
 * These times can be read by the program later. Useful for debouncing, for example.
 * 
 * @{
 * 
 * \file gpio_irq_timestamp.h
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief GPIO IRQ Timestamp header
 * \version 0.1
 * \date 2023-01-24
 */
#ifndef GPIO_IRQ_TIMESTAMP_H
#define GPIO_IRQ_TIMESTAMP_H

#include "pico/stdlib.h"
#include "pico/time.h"

/**
 * \brief Begin watching a GPIO pin for falling or rising events.
 *
 * \param gpio GPIO number to setup (<32)
 * \param events Either GPIO_IRQ_EDGE_FALL or GPIO_IRQ_EDGE_RISE
 * \return PICO_ERROR_INVALID_ARG if invalid argument passed in. Else PICO_ERROR_NONE 
 */
int gpio_irq_timestamp_setup(uint8_t gpio, uint32_t events);

/**
 * \brief Return timestamp of last interrupt event.
 * 
 * \param gpio GPIO to check
 * \return Timestamp of last interrupt event. nil_time if GPIO is out of range. 
 */
absolute_time_t gpio_irq_timestamp_read(uint8_t gpio);

/**
 * \brief Return time in microseconds since last interrupt event.
 * 
 * \param gpio GPIO to check
 * \return Duration in microseconds of last interrupt event. PICO_ERROR_INVALID_ARG if GPIO is out of range. 
 */
int64_t gpio_irq_timestamp_read_duration_us(uint8_t gpio);

/**
 * \brief Return time in milliseconds since last interrupt event.
 * 
 * \param gpio GPIO to check
 * \return Duration in milliseconds of last interrupt event. PICO_ERROR_INVALID_ARG if GPIO is out of range. 
 */
int64_t gpio_irq_timestamp_read_duration_ms(uint8_t gpio);

#endif
/** @} */