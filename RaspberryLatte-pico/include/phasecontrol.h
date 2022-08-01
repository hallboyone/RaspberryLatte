#include "pico/stdlib.h"

#define ZEROCROSS_EVENT_RISING   0x08
#define ZEROCROSS_EVENT_FALLING  0x04

/**
 * Structure holding the configuration values of a phase controller for and
 * inductive load.
 */
typedef struct {
  uint8_t event;             // RISING or FALLING
  uint8_t zerocross_pin;     // GPIO that senses event at every zerocrossing
  int64_t zerocross_shift;   // Time between zerocross trigger and actual zero cross
  uint8_t out_pin;           // Load output pin
} PhasecontrolConfig;

/**
 * \brief Setup for phasecontrol. Pins are configured and a callback is attached to the zerocross pin.
 * The phasecontroller is also registered with the UART bridge. 
 * 
 * \param user_config A constant configuration object containing the desired pin number, and other fields. 
 */
void phasecontrol_setup(const PhasecontrolConfig * user_config);

/**
 * \brief Update the duty cycle. If value is out of range (0<=val<=127), it is clipped.
 * 
 * \param duty_cycle New duty cycle value between 0 and 127 inclusive.
 */
void phasecontrol_set_duty_cycle(uint8_t duty_cycle);

/**
 * \brief Check if zerocross pin has triggered in the last 16766us (period of 60Hz signal plus 100us), 
 * indicating active AC.
 * 
 * \return true if zerocross pin triggered in the last 16,766us. False otherwise.
 */
bool phasecontrol_is_ac_hot();