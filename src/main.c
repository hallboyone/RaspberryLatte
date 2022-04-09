#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"

//#include "components/uart.h"
//#include "components/physical_inputs.h"
//#include "components/pressure_sensor.h"
 #include "components/phasecontrol.h"

//#include "hx711.pio.h"
//#include "components/phasecontrol.h"
#include "lmt01.pio.h"

void packData(uint64_t scale_val,
	      uint64_t thermo_val,
	      uint64_t switch_vals,
	      uint64_t pressure_val,
	      uint64_t * buf){
  *buf = 0;
  // 0-15
  *buf = ((thermo_val & 0x0000ff00)>>8) | ((thermo_val & 0x000000ff)<<8);
  // 16-31
  *buf = (*buf) | ((pressure_val & 0xff00) << 8) | ((pressure_val & 0x00ff) << 24);
  // 32-34
  *buf = (*buf) | switch_vals<<32;
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
 /*
  // ======== Set up digital thermo ========
  LMT01 thermo = {.pio_num = 0,
                  .sig_pin = 15};
  lmt01_setup(&thermo);

  // ======== Set up physical inputs ========
  PhysicalInputs switches = {.gpio_pump = 16,
			     .gpio_dial = {17, 18, 19, 20}};
  physical_inputs_setup(&switches);

  // ======== Set up pressure sensor ========
  PressureSensor pressure_sensor = {.a_pin = 28};
  pressure_sensor_setup(&pressure_sensor);
  */

  // ======= Set up phase constrol =======
  PhasecontrolConfig pump_config = {.event           = FALLING,
				                            .zerocross_pin   = 15,
				                            .out_pin         = 14,
				                            .zerocross_shift = 300};
  phasecontrol_setup(&pump_config);

 /*
  // ========== Set up the UART ==========
  UART pi_uart = {.id = uart1,
		  .tx_pin = 4,
		  .rx_pin = 5,
		  .baudrate = 115200};
  uart_setup(&pi_uart);
  */
  
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
    packData(0, thermo.val, switches.state, pressure_sensor.val, &payload);
    uart_send(&pi_uart, (uint8_t*)&payload, 8);
    */
    //sleep_ms(50);
    gpio_put(LED_PIN, phasecontrol_is_ac_hot());
  }
}
