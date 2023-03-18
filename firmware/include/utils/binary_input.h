/**
 * \defgroup binary_input GPIO Binary Inputs
 * \ingroup utils
 * \version 0.2
 * 
 * \brief Abstraction of configuring and reading binary inputs on the Pi Pico.
 * 
 * Using the pico's GPIOs as binary inputs requires a small amount of setup. This setup
 * becomes more complex when multiple inputs are grouped together (e.g. in a multi throw
 * switch). Debouncing these inputs is more complex still. This library abstracts all 
 * this complexity into \ref binary_input objects. These structures are setup and then
 * read as needed. The debouncing and muxing is handled automatically.
 * 
 * \todo Replace with multi-gpio interrupts when implemented. 
 * 
 * \{
 * \file
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief GPIO Binary-input Library header
 * \version 0.2
 * \date 2022-08-16
 */

#ifndef BINARY_INPUT_H
#define BINARY_INPUT_H

#include "pico/stdlib.h"

/** \brief Enumeration of the different pull options (up, down, and none) */
typedef enum {BINARY_INPUT_PULL_UP, BINARY_INPUT_PULL_DOWN, BINARY_INPUT_PULL_NONE} binary_input_pull_dir;

/** \brief Opaque object defining a binary input. */
typedef struct binary_input_s* binary_input;

/**
 * \brief Create a binary input with the indicated number of throws. 
 * 
 * This inputs are all pulled in one direction. If pulled down, then high is read as
 * on. If pulled up, then high is read as off. These rules are reversed if the
 * \ref invert flag is set. 
 * 
 * The input can be muxed (i.e. a binary number is written to the pins). If it is not,
 * then indicating pole 2 of 4 is active, for example, would require 4 GPIO pins with
 * states 0100. If it is, then this is only 2 pins with states 01 (i.e. binary for 2).
 * 
 * The input can also be debounced. This will only register changes to the input after
 * it has remained constant for some debounce time.
 *
 * \param num_pins The number of pins for the binary input.
 * \param pins Pointer to an array of GPIO pin numbers of length num_pins.
 * \param pull_dir Input pull direction.
 * \param debounce_us Length a GPIO pin has to dwell in state before reading as switched (rapid changes are ignored). TO USE
 * DEBOUNCE LOGIC, NO OTHER GPIO INTERRUPTS SHOULD BE USED!  
 * \param invert Flag indicating if the pins should be inverted. 
 * \param muxed True if digital inputs are muxed (e.g. 4 throw muxed into 2 pins)
 * 
 * \return New binary_input object.
 */
binary_input binary_input_setup(uint8_t num_pins, const uint8_t * pins, binary_input_pull_dir pull_dir, uint debounce_us, bool invert, bool muxed);

/**
 * \brief Reads the requested binary_input.
 * 
 * If switch is muxed, returns the bit mask of active pins. Else returns the index of first active pin.
 * The definition of which pin is active depends on the pull direction and if the invert flag is set.
 * 
 * \param b Index of the requested switch. Must have been setup previously. 
 * \returns If switch is not muxed, then the index of the first active pin is returned (1 indexed, 
 * 0 if no pin is found). If switch is muxed, then the the state of the pins are encoded into a 
 * uint8_t mask (i.e, second of three pins active, 010 returned).
 */
int binary_input_read(binary_input b);

void binary_input_deinit(binary_input b);

#endif

/** /} */