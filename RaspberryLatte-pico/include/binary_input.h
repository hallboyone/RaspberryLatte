#ifndef _BINARY_INPUT_H
#define _BINARY_INPUT_H

#include "pico/stdlib.h"

#include "uart_bridge.h"

#define BINARY_INPUT_PULL_UP   0
#define BINARY_INPUT_PULL_DOWN 1

/**
 * \brief Data related to a binary input. Handles multithrow switches and allows for muxed hardware.
 */
typedef struct {
    uint8_t num_pins;
    uint8_t* pins;
    bool muxed;
    bool inverted;
} binary_input;

/**
 * \brief Create a binary input with the indicated number of throws. Only one of the throws
 * can be active at a time. The state of the switch is packed into binary indicating
 * the index of the active pin.
 *
 * \param num_pins The number of pins for the binary input.
 * \param pins Pointer to an array of GPIO pin numbers of length \p num_throw.
 * \param pull_dir Set to either PULL_UP or PULL_DOWN 
 * \param invert Flag indicating if the pins should be inverted. 
 * \param muxed True if digital inputs are muxed (e.g. 4 throw muxed into 2 pins)
 * 
 * \returns A unique ID assigned to the binary input or -1 if no input was created
 */
int binary_input_setup(binary_input * b, uint8_t num_pins, const uint8_t * pins, uint8_t pull_dir, bool invert, bool muxed);

/**
 * \brief Reads the requested switch. If switch is muxed, returns the bit mask of the pins. Else
 * returns the index of first high pin. 
 * 
 * \param switch_idx Index of the requested switch. Must have been setup previously. 
 * \returns If switch is not muxed, then the index of the first active pin is returned (1 indexed, 
 * 0 if no pin is found). If switch is muxed, then the the state of the pins are encoded into a 
 * uint8_t mask (i.e, second of three pins active, 010 returned).
 */
int binary_input_read(binary_input * b);

/**
 * \brief Callback that reads the binary input pointed at by local_data and returns its value as a 1 byte
 * message over UART.
 * 
 * \param id The ID of the callback. Each registered callback must have a unique callback ID.
 * \param local_data Void pointer which MUST point at an binary_input object.
 * \param uart_data Pointer to data sent over UART. Since this is a read callback, no data is needed.
 * \param uart_data_len Number of bytes in uart_data. Since this is a read callback, this should be 0.
 */
void binary_input_uart_callback(message_id id, void * local_data, int * uart_data, int uart_data_len);
#endif