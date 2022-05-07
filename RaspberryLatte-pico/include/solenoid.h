#include "pico/stdlib.h"

/**
 * Configures a digital output and registers the message handler
 * 
 * \param pin The GPIO pin attached to SSR controling the solenoid
 */
void solenoid_setup(uint8_t pin);