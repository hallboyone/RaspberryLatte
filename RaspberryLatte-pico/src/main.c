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

float sensor_val = 1;
float latest_u = 0;

float dummy_sensor(){
  sensor_val *= -0.95;
  return sensor_val;
}

void dummy_plant(float u){
  latest_u = u;
  volatile int var = 0;
  var++;
}

int main(){
  pid_ctrl boiler_ctrl = {.K={.p = 1, .i = 0.1, .d = 0.025}, .setpoint = 0, .sensor = &dummy_sensor, .plant = &dummy_plant};
  float ub = 0.15;
  pid_gains K = {.p = 1, .i = 0.1, .d = 0.025};
  pid_init(&boiler_ctrl, 0, K, &dummy_sensor, &dummy_plant, NULL, &ub, 250);
  pid_tick(&boiler_ctrl);
  return 1;
}