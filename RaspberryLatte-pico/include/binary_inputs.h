/**
 * [!!!!OLD!!!!]Interface for a SPST and SP4T switch. The state is encoded in a 8 bit value as
 *       [7-3: Not used][2: SPST off/on][1-0: SP4T value]
 */

#include "pico/stdlib.h"

/**
 * Create a binary input with the indicated number of throws. Only one of the throws
 * can be active at a time. The state of the switch is packed into binary indicating
 * the index of the active pin.
 *
 * @param num_throw The number of throws in the binary input.
 * @param pins Pointer to an array of GPIO pin numbers of length \p num_throw.
 * @param pull_down True if pins should be pulled down. False else. 
 */
void binary_inputs_setup(uint8_t num_throw, const uint8_t * pins, bool pull_down);

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
static void physical_inputs_read_handler(int * data, int len);