#include "pico/stdlib.h"
#include "hardware/uart.h"

typedef struct UART_{
  uart_inst_t * const id;
  uint8_t rx_pin;
  uint8_t tx_pin;
  uint baudrate;
} UART;

void uart_setup(UART* uart);
void uart_send(UART* uart, uint8_t * buf, uint8_t len);
bool uart_data_in_rx(UART* uart);
void uart_read_and_clear(UART* uart, uint8_t * buf, uint8_t len);