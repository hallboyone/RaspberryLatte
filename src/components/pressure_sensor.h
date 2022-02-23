#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

typedef struct {
  uint8_t a_pin;
  uint16_t val;
} PressureSensor;


void pressure_sensor_read(PressureSensor * s){
  adc_select_input(s->a_pin - 26);
  s->val = adc_read();
}

void pressure_sensor_setup(PressureSensor * s){
  adc_init();
  adc_gpio_init(s->a_pin);
} 
