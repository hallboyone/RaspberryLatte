#ifndef _BINARY_OUTPUT_H
#define _BINARY_OUTPUT_H
/**
 * Create binary output blocks of 1 or more GPIO pins. Each block is assigned
 * a unique ID and the outputs within each block are accessed using this ID and
 * and offset into the block. 
 */

#include "pico/stdlib.h"

#include "uart_bridge.h"

typedef struct {
    uint8_t num_pins;
    uint8_t * pins;
} binary_output;

/**
 * \brief Setup a bank of binary outputs with 1 or more pins.
 *
 * \param b Pointer to binary_output object that will be setup.
 * \param pins Pointer to an array of GPIO pin numbers.
 * \param num_pins The number of pins for the binary output.
 * 
 * \returns A unique, ID assigned to the binary output. -1 if output not created.
 */
int binary_output_setup(binary_output * b, const uint8_t * pins, const uint8_t num_pins);

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
 * \brief Write the mask to the specified output block. 
 * 
 * \param b Pointer to binary_output object that will be written to.
 * \param mask Binary values to write to block.
 */
int binary_output_mask(binary_output * b, uint mask);

void binary_output_uart_callback(message_id id, void * local_data, int * uart_data, int uart_data_len);

#endif