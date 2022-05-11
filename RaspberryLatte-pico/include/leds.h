#include "pico/stdlib.h"

void leds_setup(const uint led0_pin, const uint led1_pin, const uint led2_pin);

void leds_set(uint led_idx, bool val);