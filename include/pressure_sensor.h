#include "pico/stdlib.h"

/**
 * Configures an anolog input at a_pin to read a pressure sensor and registers as a message handler.
 * 
 * \param a_pin The GPIO pin that the pressure signal is attached to. Must be GPIO 26, 27, 28, or 29.
 */
void pressure_sensor_setup(uint8_t a_pin);

/**
 * \brief Gets the value of the pressure sensor attached to configured pin.
 * 
 * \returns The raw value of the configured pressure sensor. 
 */
int pressure_sensor_read();