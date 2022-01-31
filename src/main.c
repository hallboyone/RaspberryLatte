#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"

#include "components/uart.h"
#include "components/max31855.h"
#include "hx711.pio.h"
#include "components/phasecontrol.h"

void packData(uint64_t scale_val,
	      uint64_t thermo_val,
	      uint64_t switch_vals,
	      uint64_t * buf){
  *buf = 0;
  //scale_val = 3;
  //scale_val <<= 30;
  scale_val = 0;
  //thermo_val = 0;
  switch_vals = 0;
  *buf = (((uint64_t)switch_vals)<<(24+14)) | (((uint64_t)thermo_val)<<0) | ((uint64_t)(scale_val));
}

int main(){
  // ============ Set up LED =============
  const uint LED_PIN = PICO_DEFAULT_LED_PIN;
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  // ========= Set up the scale ==========
  HX711 scale;
  const uint pio_num = 0;
  const uint dat_pin = 16;
  const uint clk_pin = 17;
  hx711_setup(&scale, pio_num, dat_pin, clk_pin);

  // ======== Set up thermocouple ========
  MAX31855 thermo = {.spi = spi1,
		     .cs  = 11,
		     .clk = 10,
		     .din = 12};
  max31855_setup(&thermo);

  // ======= Set up phase constrol =======
  PHASECONTROL_CONFIG pump_config = {.trigger         = RISING,
				     .zerocross_pin   = 15,
				     .out_pin         = 14,
				     .zerocross_delay = 1620};
  phasecontrol_setup(&pump_config);
  
  // ========== Set up the UART ==========
  UART pi_uart = {.id = uart1,
		  .tx_pin = 4,
		  .rx_pin = 5,
		  .baudrate = 115200};
  uart_setup(&pi_uart);

  
  uint8_t msg_buf[4];
  uint64_t payload = 0;
  bool led_state = false;
  int dir = 1;
  while(1){
    //if(uart_data_in_rx(&pi_uart)){
      //uart_read_and_clear(&pi_uart, msg_buf, 4);
      
      // Send a 4 byte message containing the scale value
      //uart_send(&pi_uart, (uint8_t*)&scale_val, 4);
      
    //}

    // Read sensors
    hx711_read(&scale);
    max31855_read(&thermo);
    packData(scale.val, (uint32_t)thermo.val[0], 0, &payload);
    uart_send(&pi_uart, (uint8_t*)&payload, 8);
    
    sleep_ms(1000);
    gpio_put(LED_PIN, led_state);
    led_state = !led_state;
  }
}
