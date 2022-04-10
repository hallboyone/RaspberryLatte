#include "physical_inputs.h"
#include "hardware/gpio.h"

void physical_inputs_setup(PhysicalInputs * s){
  gpio_init(s->gpio_pump);
  gpio_init(s->gpio_dial[0]);
  gpio_init(s->gpio_dial[1]);
  gpio_init(s->gpio_dial[2]);
  gpio_init(s->gpio_dial[3]);

  gpio_set_dir(s->gpio_pump, false);
  gpio_set_dir(s->gpio_dial[0], false);
  gpio_set_dir(s->gpio_dial[1], false);
  gpio_set_dir(s->gpio_dial[2], false);
  gpio_set_dir(s->gpio_dial[3], false);

  gpio_pull_down(s->gpio_pump);
  gpio_pull_down(s->gpio_dial[0]);
  gpio_pull_down(s->gpio_dial[1]);
  gpio_pull_down(s->gpio_dial[2]);
  gpio_pull_down(s->gpio_dial[3]);

  physical_inputs_read(s);
}

void physical_inputs_read(PhysicalInputs * s){
  s->state = (gpio_get(s->gpio_pump)<<2);
  for (uint i = 0; i <= 3; i++){
    if(gpio_get(s->gpio_dial[i])){
      s->state = s->state | (i);
      return;
    }
  }
}

uint8_t physical_inputs_spst(PhysicalInputs * s){
  physical_inputs_read(s);
  return ((0x04 & s->state)>>2);
}

uint8_t physical_inputs_sp4t(PhysicalInputs * s){
  physical_inputs_read(s);
  return (0x03 & s->state);
}