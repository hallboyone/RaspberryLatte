#include "pico/stdlib.h"
#include "pico/time.h"

#include "pinout.h"

#include "maintainer.h"
#include "phasecontrol.h"
#include "binary_input.h"
#include "pressure_sensor.h"
#include "uart_bridge.h"
#include "hx711.h"
#include "lmt01.h"
#include "heater.h"
#include "solenoid.h"
#include "leds.h"

bool run = true;
bool led = false;

void endProgram(int * data, int len){
  run = false;
}

#ifdef PICO_DEFAULT_LED_PIN
static bool toggle_led(repeating_timer_t *rt){
  led = !led;
  gpio_put(PICO_DEFAULT_LED_PIN, led);
  return true;
}
#endif

int main(){
  // Setup UART, clear queue, and assign endProgram command
  stdio_init_all();
  while(getchar_timeout_us(10) != PICO_ERROR_TIMEOUT) tight_loop_contents();
  registerHandler(MSG_ID_END_PROGRAM, &endProgram);

  #ifdef PICO_DEFAULT_LED_PIN
  // Setup the onboard LED
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
  repeating_timer_t led_timer;
  add_repeating_timer_ms(1000, &toggle_led, NULL, &led_timer);
  #endif

  pressure_sensor_setup(PRESSURE_SENSOR_PIN);

  const uint8_t pump_switch_gpio = PUMP_SWITCH_PIN;
  const uint8_t mode_select_gpio[4] = {DIAL_A_PIN, DIAL_B_PIN};
  binary_input_setup(1, &pump_switch_gpio, PULL_DOWN, false);
  binary_input_setup(2, mode_select_gpio, PULL_DOWN, true);
  
  // Set up phase control
  PhasecontrolConfig pump_config = 
  {.event          = FALLING,
  .zerocross_pin   = PHASE_CONTROL_0CROSS_PIN,
  .out_pin         = PHASE_CONTROL_OUT_PIN,
  .zerocross_shift = PHASE_CONTROL_0CROSS_SHIFT};
  phasecontrol_setup(&pump_config);

  hx711_setup(0, SCALE_DATA_PIN, SCALE_CLOCK_PIN);

  heater_setup(HEATER_PWM_PIN);

  lmt01_setup(0, LMT01_DATA_PIN);

  solenoid_setup(SOLENOID_PIN);
  
  leds_setup(LED0_PIN, LED1_PIN, LED2_PIN);

  // Continually look for a message and then run maintenance
  while(run){
    readMessage();
    runMaintenance();
  }
}