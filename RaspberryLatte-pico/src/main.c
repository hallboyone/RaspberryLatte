#include <stdio.h>

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

#include "pid.h"

bool run = true;
bool led = false;

void endProgram(int * data, int len){
  run = false;
}

int main(){
  stdio_init_all();

  lmt01_setup(0, LMT01_DATA_PIN);
  heater_setup(HEATER_PWM_PIN);

  pid_ctrl boiler_ctrl = {.setpoint = 95, .K = {.p = 0.05, .i = 0.0015, .d = 0.000}, 
                          .sensor = &lmt01_read_float, .plant = &heater_pid_input,
                          .min_time_between_ticks_ms = 100};
  pid_init(&boiler_ctrl, 0, 100, 0);

  while(true){
    pid_tick(&boiler_ctrl);
  }

  return 1;
}