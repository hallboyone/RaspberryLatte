/**
 * Create binary output blocks of 1 or more GPIO pins. Each block is assigned
 * a unique ID and the outputs within each block are accessed using this ID and
 * and offset into the block. 
 */

#include "pico/stdlib.h"

/**
 * \brief Setup a bank of binary outputs with 1 or more pins.
 *
 * \param pins Pointer to an array of GPIO pin numbers.
 * \param num_pins The number of pins for the binary output.
 * 
 * \returns A unique, ID assigned to the binary output. -1 if output not created.
 */
int binary_output_setup(const uint8_t * pins, const uint8_t num_pins);

/**
 * \brief Write val to the specified GPIO pin.
 * 
 * \param id ID of block containing desired GPIO pin.
 * \param offset Offset into the block containing the desired GPIO pin.
 * \param val Binary value to write to GPIO.
 * 
 * \returns True if operation was successful. False else.
 */
bool binary_output_put(uint8_t id, uint8_t offset, bool val);

/**
 * \brief Write the mask to the specified output block. 
 * 
 * \param id ID of block to write to.
 * \param mask Binary values to write to block.
 */
void binary_output_mask(uint8_t id, uint mask);