/**
 * \file
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Abstraction of configuring and reading binary inputs on the Pi Pico
 * \version 0.1
 * \date 2022-08-16
 */

#ifndef BINARY_INPUT_H
#define BINARY_INPUT_H

#include "pico/stdlib.h"

#define BINARY_INPUT_PULL_UP   0
#define BINARY_INPUT_PULL_DOWN 1

/**
 * \brief Data related to a binary input. Handles multithrow switches and allows for muxed hardware.
 */
typedef struct {
    uint8_t num_pins; /**< How many GPIO pins are used in binary input. */
    uint8_t* pins;    /**< Array of pin numbers used in binary input. */
    bool muxed;       /**< Flag indicating if the input is muxed (pins read as binary numer). */
    bool inverted;    /**< Flag indicating if the wiring requires the pins to be inverted. */
} binary_input;

/**
 * \brief Create a binary input with the indicated number of throws. Only one of the throws
 * can be active at a time. The state of the switch is packed into binary indicating
 * the index of the active pin.
 *
 * \param b Pointer to binary_input stuct that will store information for binary input.
 * \param num_pins The number of pins for the binary input.
 * \param pins Pointer to an array of GPIO pin numbers of length num_pins.
 * \param pull_dir Set to either BINARY_INPUT_PULL_UP or BINARY_INPUT_PULL_DOWN 
 * \param invert Flag indicating if the pins should be inverted. 
 * \param muxed True if digital inputs are muxed (e.g. 4 throw muxed into 2 pins)
 */
void binary_input_setup(binary_input * b, uint8_t num_pins, const uint8_t * pins, uint8_t pull_dir, bool invert, bool muxed);

/**
 * \brief Reads the requested switch. If switch is muxed, returns the bit mask of the pins. Else
 * returns the index of first high pin. 
 * 
 * \param b Index of the requested switch. Must have been setup previously. 
 * \returns If switch is not muxed, then the index of the first active pin is returned (1 indexed, 
 * 0 if no pin is found). If switch is muxed, then the the state of the pins are encoded into a 
 * uint8_t mask (i.e, second of three pins active, 010 returned).
 */
int binary_input_read(binary_input * b);
#endif