#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "uart_bridge.h"

typedef struct{
    PIO _pio;
    uint _sm;
    uint8_t _dat_pin;
    int _latest_temp;
} lmt01;

/**
 * \brief Configures the signal pin attached to a LMT01 temp sensor and starts a PIO 
 * program that counts the sensors pulse train
 * 
 * \param pio_num Either 0 or 1 indicating if PIO #0 or #1 should be used
 * \param dat_pin Pin that the LMT01 is attached to
 */
void lmt01_setup(lmt01 * l, uint8_t pio_num, uint8_t dat_pin);

/**
 * \brief Returns the current temputature in 16*C. Divide by 16 6o conver to C
 */
int lmt01_read(lmt01 * l);

/**
 * \brief Returns the current tempurature in C.
 */
float lmt01_read_float(lmt01 * l);

/**
 * \brief Callback that reads the current tempurature and returns it as a 2 byte value over UART
 * 
 * \param id The ID of the callback. Each registered callback must have a unique callback ID.
 * \param local_data Void pointer which MUST point at an binary_input object.
 * \param uart_data Pointer to data sent over UART. Since this is a read callback, no data is needed.
 * \param uart_data_len Number of bytes in uart_data. Since this is a read callback, this should be 0.
 */
void lmt01_read_uart_callback(message_id id, void * local_data, int * uart_data, int uart_data_len);