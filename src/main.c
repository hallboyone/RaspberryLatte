#include "pico/stdlib.h"
#include "pico/time.h"

//#include "phasecontrol.h"
//#include "physical_inputs.h"
//#include "pressure_sensor.h"
#include "uart_bridge.h"
//#include "hx711.pio.h"
//#include "lmt01.pio.h"

void controlLED(int * data, int len){
  gpio_put(PICO_DEFAULT_LED_PIN, *data != 0);
}

int main(){
  // Setup UART and clear queue
  stdio_init_all();
  while(getchar_timeout_us(0) != PICO_ERROR_TIMEOUT) tight_loop_contents();
  
  assignHandler(0, &controlLED);
  
  // ============ Set up LED =============
  const uint LED_PIN = PICO_DEFAULT_LED_PIN;
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

/*
  // ========= Set up the scale ==========
  HX711 scale = {.pio_num = 0,
                 .dat_pin = 16,
                 .clk_pin = 17};
  //hx711_setup(&scale);

  // ======== Set up pressure sensor ========
  PressureSensor pressure_sensor = {.a_pin = 28};
  //pressure_sensor_setup(&pressure_sensor);

  // ======== Set up physical inputs ========
  PhysicalInputs switches = {.gpio_pump = 16,
			                       .gpio_dial = {17, 18, 19, 20}};
  physical_inputs_setup(&switches);

  // ======== Set up digital thermo ========
  LMT01 thermo = {.pio_num = 0,
                  .sig_pin = 15};
  //lmt01_setup(&thermo);

  // ======= Set up phase control =======
  PhasecontrolConfig pump_config = {.event           = FALLING,
				                            .zerocross_pin   = 15,
				                            .out_pin         = 14,
				                            .zerocross_shift = 300};
  //phasecontrol_setup(&pump_config);
*/
  readMessages(50);
}
