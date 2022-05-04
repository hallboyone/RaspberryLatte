/**
 * Handles the setup and reading of up to 8 multi-throw switches. Each switch
 * is setup by calling <binary_inputs_setup> with its throw count, pins, and
 * pull-down settings. This also registers an internal packer function as the
 * UART handler for MSG_ID_GET_SWITCH messages.
 */

#include "pico/stdlib.h"


#define PULL_UP   0
#define PULL_DOWN 1

/**
 * Create a binary input with the indicated number of throws. Only one of the throws
 * can be active at a time. The state of the switch is packed into binary indicating
 * the index of the active pin.
 *
 * @param num_pins The number of pins for the binary input.
 * @param pins Pointer to an array of GPIO pin numbers of length \p num_throw.
 * @param pull_dir Set to either PULL_UP or PULL_DOWN 
 * @param muxed True if digital inputs are muxed (e.g. 4 throw muxed into 2 pins)
 */
void binary_input_setup(uint8_t num_pins, const uint8_t * pins, uint8_t pull_dir, bool muxed);

/**
 * Reads the requested switch. If switch is muxed, returns the bit mask of the pins. Else returns the index of first high pin
 * 
 * @param switch_idx Index of the requested switch. Must have been setup previously. 
 * @returns If switch is not muxed, then the index of the first active pin is returned (1 indexed, 0 if no pin is found)
 * If switch is muxed, then the the state of the pins are encoded into a uint8_t mask (i.e, second of three pins active, 010 returned)
 */
uint8_t binary_input_read(uint8_t switch_idx);