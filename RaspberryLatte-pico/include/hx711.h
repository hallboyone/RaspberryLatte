#include "pico/stdlib.h"

void hx711_setup(uint8_t pio_num, uint8_t dat_pin, uint8_t clk_pin);

static void hx711_maintainer();

static void hx711_read_handler(int * msg, int len);