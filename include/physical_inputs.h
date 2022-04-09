#include "pico/stdlib.h"

typedef struct {
  uint8_t gpio_pump;
  uint8_t gpio_dial[4];
  uint8_t state;
} PhysicalInputs;

void physical_inputs_read(PhysicalInputs * s);

void physical_inputs_setup(PhysicalInputs * s);
