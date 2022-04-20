#include "pico/stdlib.h"
#include "pico/time.h"

//#include "phasecontrol.h"
//#include "physical_inputs.h"
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
  // Setup UART and clear queue
  stdio_init_all();
  while(getchar_timeout_us(10) != PICO_ERROR_TIMEOUT) tight_loop_contents();
  
  assignHandler(MSG_ID_END_PROGRAM, &endProgram);
  assignHandler(MSG_ID_LED_TEST, &controlLED);
  assignHandler(MSG_ID_RESPONSE_TEST, &sendResponse);

  // ============ Set up LED =============
  const uint LED_PIN = PICO_DEFAULT_LED_PIN;
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  // ======== Set up pressure sensor ========
  pressure_sensor_setup(28);

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
