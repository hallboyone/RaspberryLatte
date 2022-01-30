#include "pico/stdlib.h"
#include "hardware/spi.h"


typedef struct MAX31855_{
  spi_inst_t* spi;
  const uint8_t cs;
  const uint8_t clk;
  const uint8_t din;
  uint8_t val[4];
} MAX31855;

void max31855_setup(MAX31855* s){
  spi_init(s->spi, 1000000);

  spi_set_format(s->spi, 8, 0, 0, SPI_MSB_FIRST);

  gpio_set_function(s->clk, GPIO_FUNC_SPI);
  gpio_set_function(s->din, GPIO_FUNC_SPI);
  gpio_init(s->cs);
  gpio_set_dir(s->cs, GPIO_OUT);
  gpio_put(s->cs, 1);
}

void max31855_read(MAX31855* s){
  gpio_put(s->cs, 0);
  spi_read_blocking(s->spi, 0, s->val, 4);
  gpio_put(s->cs, 1);
}
