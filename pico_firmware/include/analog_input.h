#ifndef _ANALOG_INPUT_H
#define _ANALOG_INPUT_H
#include "pico/stdlib.h"

#include "uart_bridge.h"

typedef struct{
    uint8_t pin;
} analog_input;

/**
 * Configures an anolog input at a_pin.
 * 
 * \param a Pointer to analog_input that will be setup.
 * \param a_pin The GPIO pin for the analog input. Must be GPIO 26, 27, 28, or 29.
 * 
 * \returns 1 on success. 0 on failure (invalid GPIO is the most likly cause)
 */
int analog_input_setup(analog_input * a, uint8_t a_pin);

/**
 * \brief Gets the value of the analog input.
 * 
 * \param u Previously setup analog input.
 * 
 * \returns The raw value of the analog input. 
 */
uint16_t analog_input_read(analog_input * a);

/**
 * \brief Callback that reads the analog input pointed at by local_data and returns its value as a 2 byte
 * message over UART.
 * 
 * \param id The ID of the callback. Each registered callback must have a unique callback ID.
 * \param local_data Void pointer which MUST point at an analog_input object.
 * \param uart_data Pointer to data sent over UART. Since this is a read callback, no data is needed.
 * \param uart_data_len Number of bytes in uart_data. Since this is a read callback, this should be 0.
 */
void analog_input_uart_callback(message_id id, void * local_data, int * uart_data, int uart_data_len);
#endif