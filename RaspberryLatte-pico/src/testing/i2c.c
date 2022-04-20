#include "pico/stdlib.h"
#include "hardware/i2c.h"

int main() {
  const uint LED_PIN = PICO_DEFAULT_LED_PIN;

  i2c_init(i2c_default, 400000);
  i2c_set_slave_mode(i2c_default, true, 0x40);
  gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);


  while (true) {
    gpio_put(LED_PIN, 1);
    sleep_ms(250);
    gpio_put(LED_PIN, 0);
    sleep_ms(250);
  }
}
