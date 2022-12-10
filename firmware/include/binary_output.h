/**
 * \defgroup binary_output GPIO Binary Outputs
 * \ingroup drivers
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

/**
 * \brief Struct containing information for a single output block.
 */
typedef struct {
    uint8_t num_pins; /**< The number of pins in output block. */
    uint8_t * pins;   /**< Pointer to array of pin numbers for output block. */
} binary_output;

/**
 * \brief Setup a bank of binary outputs with 1 or more pins.
 *
 * \param b Pointer to binary_output object that will be setup.
 * \param pins Pointer to an array of GPIO pin numbers.
 * \param num_pins The number of pins for the binary output.
 */
void binary_output_setup(binary_output * b, const uint8_t * pins, const uint8_t num_pins);

/**
 * \brief Write val to the specified GPIO pin.
 * 
 * \param b Pointer to binary_output object that will be written to.
 * \param idx Which of the pins in b to write to.
 * \param val Binary value to write to GPIO.
 * 
 * \returns True if operation was successful. False else.
 */
int binary_output_put(binary_output * b, uint8_t idx, bool val);

/**
 * \brief Write the bits of mask to the outputs in the binary_output block b.
 * 
 * \param b Pointer to binary_output object that will be written to.
 * \param mask Bitmask that will be written to block's outputs.
 * 
 * \returns True if operation was successful. False else.
 */
int binary_output_mask(binary_output * b, uint mask);
#endif
/** \} */