#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"

#include "components/uart.h"
//#include "hx711.pio.h"
//#include "components/phasecontrol.h"
#include "lmt01.pio.h"

void packData(uint64_t scale_val,
	      uint64_t thermo_val,
	      uint64_t switch_vals,
	      uint64_t * buf){
  *buf = 0;
  // The bytes of the thermo value are reversed
  *buf = ((thermo_val & 0xff000000)>>24) | ((thermo_val & 0x00ff0000)>>8) |
         ((thermo_val & 0x0000ff00)<<8) | ((thermo_val & 0x000000ff)<<24); 
  //scale_val = 3;
  //scale_val <<= 30;
  //thermo_val = 0; 
  //*buf = (((uint64_t)switch_vals)<<(24+14)) | (((uint64_t)thermo_val)) | ((uint64_t)(scale_val));
}

int main(){
  // ============ Set up LED =============
  const uint LED_PIN = PICO_DEFAULT_LED_PIN;
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  /*
  // ========= Set up the scale ==========
  HX711 scale;
  const uint pio_num = 0;
  const uint dat_pin = 16;
  const uint clk_pin = 17;
  hx711_setup(&scale, pio_num, dat_pin, clk_pin);
  */
  // ======== Set up digital thermo ========
  LMT01 thermo = {.pio_num = 1,
                  .sig_pin = 15};
		  lmt01_setup(&thermo);
		  //gpio_init(15);
		  //gpio_set_dir(15, GPIO_IN);
  
  /*
  // ======= Set up phase constrol =======
  PHASECONTROL_CONFIG pump_config = {.trigger         = RISING,
				     .zerocross_pin   = 15,
				     .out_pin         = 14,
				     .zerocross_delay = 1620};
vv  phasecontrol_setup(&pump_config);
  */
  // ========== Set up the UART ==========
  UART pi_uart = {.id = uart1,
		  .tx_pin = 4,
		  .rx_pin = 5,
		  .baudrate = 115200};
  uart_setup(&pi_uart);

  
  //uint8_t msg_buf[4];
  uint64_t payload = 0;
  //bool led_state = false;
  //int dir = 1;
  while(1){
    //if(uart_data_in_rx(&pi_uart)){
      //uart_read_and_clear(&pi_uart, msg_buf, 4);
      
      // Send a 4 byte message containing the scale value
      //uart_send(&pi_uart, (uint8_t*)&scale_val, 4);
      
    //}
    /*
    uint32_t counter = 0;
    // Go high for at least 25ms
    absolute_time_t sample_time = get_absolute_time();
    while(absolute_time_diff_us(sample_time, get_absolute_time()) < 25000){
      if(!gpio_get(15)){
	while(!gpio_get(15)){
	  tight_loop_contents();
	}
	sample_time = get_absolute_time();
      }
    }
    while(gpio_get(15)){
      tight_loop_contents();
    }
    sample_time = get_absolute_time();
    // Count pulses till signal high for 25ms
    while(absolute_time_diff_us(sample_time, get_absolute_time()) < 25000){
      if(!gpio_get(15)){
	counter += 1;
	while(!gpio_get(15)){
	  tight_loop_contents();
	}
	sample_time = get_absolute_time();
      }
      }*/
    // Read sensors
    //hx711_read(&scale);
    lmt01_read(&thermo);
    packData(0, thermo.val, 0, &payload);
    uart_send(&pi_uart, (uint8_t*)&payload, 8);
    //uart_send(&pi_uart, (uint8_t*)&counter, 1);
    //sleep_ms(50);
    gpio_put(LED_PIN, gpio_get(15));
    //led_state = !led_state;
  }
}
