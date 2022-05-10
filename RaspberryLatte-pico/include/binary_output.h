/**
 * Handles the setup and writing of gpio outputs over uart
 */

#include "pico/stdlib.h"

/**
 * Create a binary output attached to the indicated pin. The first time this function is called, a binary output write 
 * handler is registered with the uart bridge. 
 */
void binary_output_setup(const uint pin);

/**
 * Reads the requested switch. If switch is muxed, returns the bit mask of the pins. Else returns the index of first high pin
 * 
 * @param switch_idx Index of the requested switch. Note this is not a GPIO number!
 * @param val Value to write to output.
  */
void binary_output_write(uint switch_idx, bool val);