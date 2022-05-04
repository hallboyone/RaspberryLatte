/**
 * Handles the setup and reading of up to 8 multi-throw switches. Each switch
 * is setup by calling <binary_inputs_setup> with its throw count, pins, and
 * pull-down settings. This also registers an internal packer function as the
 * UART handler for MSG_ID_GET_SWITCH messages.
 */

#include "pico/stdlib.h"

/**
 * Create a binary input with the indicated number of throws. Only one of the throws
 * can be active at a time. The state of the switch is packed into binary indicating
 * the index of the active pin.
 *
 * @param num_pins The number of pins for the binary input.
 * @param pins Pointer to an array of GPIO pin numbers of length \p num_throw.
 * @param pull_down True if pins should be pulled down. False else. 
 * @param muxed True if digital inputs are muxed (e.g. 4 throw muxed into 2 pins)
 */
void binary_input_setup(uint8_t num_pins, const uint8_t * pins, bool pull_down, bool muxed);

/**
 * Reads the requested switch.
 * 
 * @param switch_idx Index of the requested switch. Must have been setup previously. 
 * @returns 0 if non of the switches throws are triggered. Else returns the first triggered throw (Note: 1 indexed).
 */
uint8_t readSwitch(uint8_t switch_idx);
 
/**
 * Read the physical inputs and return their values over UART
 * 
 * @param data Pointer to switch indicies to read. If empty, return all.
 * @param len Number of indicies in data array. 
 */
static void binary_input_read_handler(int * data, int len);