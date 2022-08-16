/**
 * \file
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Abstraction of configuring and reading analog inputs on the Pi Pico
 * \version 0.1
 * \date 2022-08-16
 */

#ifndef _ANALOG_INPUT_H
#define _ANALOG_INPUT_H
#include "pico/stdlib.h"


typedef struct _analog_input{
    uint8_t pin; /**< The GPIO pin attached to the analog input. Must be 26-29. */
} analog_input;

/*!
 * Configures an anolog input at a_pin.
 * 
 * \param a Pointer to analog_input that will be setup.
 * \param a_pin The GPIO pin for the analog input. Must be GPIO 26, 27, 28, or 29.
 * 
 * \returns 1 on success. 0 on failure (invalid GPIO is the most likly cause)
 */
int analog_input_setup(analog_input * a, uint8_t a_pin);

/**
 * \brief Gets the value of the analog input.
 * 
 * \param a Previously setup analog input.
 * 
 * \returns The raw value of the analog input. 
 */
uint16_t analog_input_read(analog_input * a);
#endif