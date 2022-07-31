#include "pico/stdlib.h"

/**
 * \brief Sets up three GPIO pins coorisponding to the parameters to activiate external LEDs
 * \param led0_pin GPIO pin number for LED 0. 
 * \param led1_pin GPIO pin number for LED 1. 
 * \param led2_pin GPIO pin number for LED 2. 
 */
void leds_setup(const uint led0_pin, const uint led1_pin, const uint led2_pin);

/**
 * \brief Set the
 */
void leds_set(uint led_idx, bool val);