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
  uint8_t event;             ///< RISING or FALLING
  uint8_t zerocross_pin;     ///< GPIO that senses event at every zerocrossing
  int64_t zerocross_shift;   ///< Time between zerocross trigger and actual zero cross
  uint8_t out_pin;           ///< Load output pin
} PhasecontrolConfig;

/**
 * Called from core 0. Launches core 1 and passes it the required data.
 * 
 * @param[in] user_config Configuration data with pin numbers, trigger event type, and zerocross shift values. 
 */
void phasecontrol_setup(PhasecontrolConfig * user_config);

/**
 * Set duty cycle for phase controller.
 * 
 * @param[in] duty_cycle A value from -1 to 127 where -1 is off and 127 is full on. 
 */
void phasecontrol_set_duty_cycle(int8_t duty_cycle);

/**
 * Checks if the zerocross pin has changed within 1 period of a 60Hz sine wave
 * 
 * @returns True if zerocrossing had been recorded within 16.67ms. False otherwise. 
 */
bool phasecontrol_is_ac_hot();