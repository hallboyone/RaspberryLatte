#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"

#include "phasecontrol.h"
#include "physical_inputs.h"
#include "pressure_sensor.h"
#include "packer.h"
#include "uart.h"
#include "hx711.pio.h"
#include "lmt01.pio.h"


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
  

  // ======== Set up pressure sensor ========
  PressureSensor pressure_sensor = {.a_pin = 28};
  pressure_sensor_setup(&pressure_sensor);

  // ======== Set up physical inputs ========
  PhysicalInputs switches = {.gpio_pump = 16,
			     .gpio_dial = {17, 18, 19, 20}};
  physical_inputs_setup(&switches);

  // ======== Set up digital thermo ========
  LMT01 thermo = {.pio_num = 0,
                  .sig_pin = 15};
  lmt01_setup(&thermo);

  // ======= Set up phase constrol =======
  PhasecontrolConfig pump_config = {.event           = FALLING,
				                            .zerocross_pin   = 15,
				                            .out_pin         = 14,
				                            .zerocross_shift = 300};
  phasecontrol_setup(&pump_config);

 
  // ========== Set up the UART ==========
  UART pi_uart = {.id = uart1,
		  .tx_pin = 4,
		  .rx_pin = 5,
		  .baudrate = 115200};
  uart_setup(&pi_uart);
  
  
  //uint8_t msg_buf[4];
  uint64_t payload = 0;
  bool led_state = false;
  //int dir = 1;
  while(1){
    //if(uart_data_in_rx(&pi_uart)){
      //uart_read_and_clear(&pi_uart, msg_buf, 4);
      
      // Send a 4 byte message containing the scale value
      //uart_send(&pi_uart, (uint8_t*)&scale_val, 4);
      
    //}

    // Read sensors
    //hx711_read(&scale);
    /*
    lmt01_read(&thermo);
    physical_inputs_read(&switches);
    pressure_sensor_read(&pressure_sensor);
    
    uart_send(&pi_uart, (uint8_t*)&payload, 8);
    */
    //sleep_ms(50);
    packData(0, 1, 2, 3, &payload);
    gpio_put(LED_PIN, 1);
  }
}
