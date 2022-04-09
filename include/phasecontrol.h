/**
 * Reads a duty cycle values from 0 to 100 over i2c and switches an ssr 
 * accordingly relative to zerocross times. Designed to provide adjustment
 * to ac-driven inductive loads where simple pwm with and ssr would result
 * in inductive voltage spikes
 */

#include "pico/stdlib.h"

#define RISING             0x08
#define FALLING            0x04

typedef struct {
  uint8_t event;             // RISING or FALLING
  uint8_t zerocross_pin;     // GPIO that senses event at every zerocrossing
  int64_t zerocross_shift;   // Time between zerocross trigger and actual zero cross
  uint8_t out_pin;           // Load output pin
} PhasecontrolConfig;

/**
 * Called from core 0. Launches core 1 and passes it the required data.
 */
void phasecontrol_setup(PhasecontrolConfig * config_);

/**
 * Write the target duty cycle from -1 to 127 to core1. -1 is off. 
 */
void phasecontrol_set_duty_cycle(int8_t duty_cycle);

/**
 * Returns true if a zerocrossing has been sensed in the last 16,666 us
 */
bool phasecontrol_is_ac_hot();