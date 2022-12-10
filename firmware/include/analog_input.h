/**
 * \defgroup analog_input GPIO Analog Inputs
 * \ingroup drivers
 * \version 0.2
 * 
 * \brief Abstraction of configuring and reading analog inputs on the Pi Pico.
 * 
 * Library providing matching interface design for analog inputs as the \ref binary_input 
 * library provides for binary inputs. Abstracts away the GPIO setup and handles unit
 * conversion.
 * 
 * \{
 * \file
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Analog Input Library header
 * \version 0.1
 * \date 2022-08-16
 */

#ifndef ANALOG_INPUT_H
#define ANALOG_INPUT_H
#include "pico/stdlib.h"


/**
 * \brief A struct for a single analog input. 
 */
typedef struct {
    uint8_t pin; /**< \brief The GPIO pin attached to the analog input. Must be 26-29. */
    float conversion_factor; /**< \brief floating point conversion factor. */
} analog_input;

/*!
 * Configures an analog input at a_pin.
 * 
 * \param a Pointer to analog_input that will be setup.
 * \param a_pin The GPIO pin for the analog input. Must be GPIO 26, 27, 28, or 29.
 * \param conversion_factor Floating point conversion factor.
 * 
 * \returns PICO_ERROR_NONE on success. Error code on failure (invalid GPIO is the most likely cause)
 */
int analog_input_setup(analog_input * a, uint8_t a_pin, float conversion_factor);

/**
 * \brief Gets the value of the analog input with conversion.
 * 
 * \param a Previously setup analog input.
 * 
 * \returns The converted value of the analog input. 
 */
float analog_input_read(analog_input * a);

/**
 * \brief Gets the value of the analog input without conversion.
 * 
 * \param a Previously setup analog input.
 * 
 * \returns The raw value of the analog input. 
 */
uint16_t analog_input_read_raw(analog_input * a);
#endif