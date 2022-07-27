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
  discrete_integral i;
  const float ub = 5;
  discrete_integral_init(&i, NULL, &ub);
  volatile float val = 0;
  datapoint p = {.v = 0, .t = 10};
  val = discrete_integral_add_point(&i, p);
  p.v = 1;
  p.t = 11;
  val = discrete_integral_add_point(&i, p);
  p.v = 2;
  p.t = 12;
  val = discrete_integral_add_point(&i, p);
  p.v = 2;
  p.t = 13;
  val = discrete_integral_add_point(&i, p);
  p.v = 4;
  p.t = 15;
  val = discrete_integral_add_point(&i, p);
  val = val + 1;
  return 1;
}