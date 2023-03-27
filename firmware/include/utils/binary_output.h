/**
 * \defgroup binary_output GPIO Binary Outputs
 * \ingroup utils
 * \brief Abstraction of configuring and writing binary outputs on the Pi Pico.
 * 
 * Create binary output blocks of 1 or more GPIO pins. Each block is assigned
 * a unique ID and the outputs within each block are accessed using this ID and
 * and offset into the block. 
 * 
 * \{
 * 
 * \file
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Binary Output header
 * \version 0.1
 * \date 2022-08-16
 */

#ifndef BINARY_OUTPUT_H
#define BINARY_OUTPUT_H

#include "pico/stdlib.h"

/** \brief Opaque object defining a binary output. */
typedef struct binary_output_s* binary_output;

/**
 * \brief Setup a bank of binary outputs with 1 or more pins.
 *
 * \param b Pointer to binary_output object that will be setup.
 * \param pins Pointer to an array of GPIO pin numbers.
 * \param num_pins The number of pins for the binary output.
 * 
 * \returns New binary_output object.
 */
binary_output binary_output_setup(const uint8_t * pins, const uint8_t num_pins);

/**
 * \brief Write val to the specified GPIO pin.
 * 
 * \param b Pointer to binary_output object that will be written to.
 * \param idx Which of the pins in b to write to.
 * \param val Binary value to write to GPIO.
 * 
 * \returns True if operation was successful. False else.
 */
int binary_output_put(binary_output b, uint8_t idx, bool val);

/**
 * \brief Write the bits of mask to the outputs in the binary_output block b.
 * 
 * \param b Pointer to binary_output object that will be written to.
 * \param mask Bitmask that will be written to block's outputs.
 * 
 * \returns True if operation was successful. False else.
 */
int binary_output_mask(binary_output b, uint mask);

/**
 * \brief Free memory storing passed in binary_output object
 * 
 * \param b Binary output object to deinit
*/
void binary_output_deinit(binary_output b);

#endif
/** \} */