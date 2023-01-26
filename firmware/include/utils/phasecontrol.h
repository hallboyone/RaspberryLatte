/**
 * \defgroup phasecontrol Phase Control Library
 * \ingroup utils
 * \brief Provides PWM like signal with switches timed relative to zero-crossings in AC voltage.
 * 
 * In circuits, switching inductive loads requires special consideration. Switching off when the
 * current is high can lead to sudden spikes in the voltage, causing arcking and component damage.
 * Even with circuits designed accordingly, constant PWM signals can create harmonics in the amount
 * of power delivered to the load. This library gets around this problem by timing the switches 
 * based on the AC signal's zero-cross time. 
 * 
 * This library was designed specifically for vibratory pumps used in espresso machines. These have 
 * a diode so the pump is only active for half the period of the wave. When current is flowing through
 * the diode, the load is not inductive so the system can be safely switched off. If used in a load
 * without a diode, the timing after a zero cross when the current goes to zero must be tuned. 
 * 
 * \todo Switch to multiple GPIO callback library when written.
 * 
 * \{
 * 
 * \file
 * \author Richard Hall (hallboyone@icloud.com)
 * \version 0.1
 * \date 2022-08-16
 * \brief Phase Control header
 */

#ifndef PHASECONTROL_H
#define PHASECONTROL_H

#include "pico/stdlib.h"

#define ZEROCROSS_EVENT_RISING   0x08 /**< Macro used to indicate zerocross occurs on rising edge of signal */
#define ZEROCROSS_EVENT_FALLING  0x04 /**< Macro used to indicate zerocross occurs on falling edge of signal */

/**
 * Structure holding the configuration values of a phase controller for and
 * inductive load.
 */
typedef struct {
  uint8_t event;             /**< The event to trigger on. Should be either ZEROCROSS_EVENT_RISING or ZEROCROSS_EVENT_FALLING */
  uint8_t zerocross_pin;     /**< GPIO attached to zerocross circuit */
  int64_t zerocross_shift;   /**< Time between zerocross trigger and actual zerocross */
  uint8_t out_pin;           /**< Load output pin. Usually attached to an SSR or relay */

  uint64_t _zerocross_time;  /**< Time of the last zero-crossing. Used to determine if the AC is on. */
  uint8_t _timeout_idx;      /**< The timeout (i.e. duty cycle). The smaller the number, the longer before load is switched on. */
} phasecontrol;

/**
 * \brief Setup for phasecontrol. Pins are configured and a callback is attached to the zerocross pin.
 * 
 * \param p A pointer to a phasecontrol struct representing the object.
 * \param zerocross_pin Pin that senses zero crossing
 * \param out_pin Pin that switches the load
 * \param zerocross_shift Time in us that the zerocross is from the sensing time.
 * \param event Event to trigger zerocross on. Either ZEROCROSS_EVENT_RISING or ZEROCROSS_EVENT_FALLING. 
 */
void phasecontrol_setup(phasecontrol * p, uint8_t zerocross_pin, uint8_t out_pin, int32_t zerocross_shift, uint8_t event);

/**
 * \brief Update the duty cycle. If value is out of range (0<=val<=127), it is clipped.
 * 
 * \param p Pointer to phase control object that will be updated
 * \param duty_cycle New duty cycle value between 0 and 127 inclusive.
 */
int phasecontrol_set_duty_cycle(phasecontrol * p, uint8_t duty_cycle);

/**
 * \brief Check if zerocross pin has triggered in the last 16766us (period of 60Hz signal plus 100us), 
 * indicating active AC.
 * 
 * \return true if zerocross pin triggered in the last 16,766us. False otherwise.
 */
bool phasecontrol_is_ac_hot(phasecontrol * p);
#endif
/** \} */