#include "pico/stdlib.h"
#include "pico/time.h"

//#include "phasecontrol.h"
#include "binary_inputs.h"
#include "pressure_sensor.h"
#include "uart_bridge.h"
//#include "hx711.pio.h"
//#include "lmt01.pio.h"

bool run = true;

void controlLED(int * data, int len){
  gpio_put(PICO_DEFAULT_LED_PIN, *data != 0);
}

void endProgram(int * data, int len){
  run = false;
}

void sendResponse(int * data, int len){
  int my_response[len];
  // Flip order
  for (int i=0; i<len; i++){
    my_response[i] = data[len-i-1];
  }
  sendMessage(MSG_ID_RESPONSE_TEST, my_response, len);
}

int main(){
  // Setup UART, clear queue, and assign endProgram command
  stdio_init_all();
  while(getchar_timeout_us(10) != PICO_ERROR_TIMEOUT) tight_loop_contents();
  assignHandler(MSG_ID_END_PROGRAM, &endProgram);

  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

  pressure_sensor_setup(28);

  const uint8_t pump_switch_gpio = 16;
  const uint8_t mode_select_gpio[4] = {17,18,19,20};
  binary_inputs_setup(1, &pump_switch_gpio, true);
  binary_inputs_setup(4, mode_select_gpio, true);
  
  // Open UART bridge
  while(run){
    readMessage();
  }
/*
  // ========= Set up the scale ==========
  HX711 scale = {.pio_num = 0,
                 .dat_pin = 16,
                 .clk_pin = 17};
  //hx711_setup(&scale);

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
}
