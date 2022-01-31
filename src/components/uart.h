#include "pico/stdlib.h"
#include "hardware/uart.h"

#define DATA_BITS        8
#define STOP_BITS        1
#define PARITY           UART_PARITY_NONE

typedef struct UART_{
  uart_inst_t * const id;
  uint8_t rx_pin;
  uint8_t tx_pin;
  uint baudrate;
} UART;

void uart_setup(UART* uart){
  // Put the uart in a known state and enable it.
  uart->baudrate = uart_init(uart->id, uart->baudrate);
  
  gpio_set_function(uart->tx_pin, GPIO_FUNC_UART);
  gpio_set_function(uart->rx_pin, GPIO_FUNC_UART);

  // Do we want to use the CTS and RTS lines?
  bool use_cts = false; // Clear-to-send line
  bool use_rts = false; // Request-to-send line
  uart_set_hw_flow(uart->id, use_cts, use_rts);

  // Set value with 8 data bits, 1 stop bit, and no parity
  uart_set_format(uart->id, DATA_BITS, STOP_BITS, PARITY);
  uart_set_fifo_enabled(uart->id, true);
}

void uart_send(UART* uart, uint8_t * buf, uint8_t len){
  uart_write_blocking(uart->id, buf, len);
}

bool uart_data_in_rx(UART* uart){
  return uart_is_readable(uart->id);
}

void uart_read_and_clear(UART* uart, uint8_t * buf, uint8_t len){
  // Fill buf with first len chars and drop the rest
  uart_read_blocking(uart->id, buf, len);
  while(uart_is_readable(uart->id)){
    uart_getc(uart->id);
  }
} 
