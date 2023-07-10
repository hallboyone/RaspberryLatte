/**
 * \defgroup slow_pwm Slow PWM Library
 * \ingroup utils
 * \brief Sets up a low frequency (~0.5Hz) PWM signal
 * 
 * Uses the alarm functions built into the rp2040 SDK. Useful in cases where fast switching
 * would be undesirable such as zerocross SSRs.
 * 
 * @{
 * \file slow_pwm.h
 * \brief Slow PWM header
 * \author Richard Hall (hallboyone@icloud.com)
 */

#ifndef SLOW_PWM_H
#define SLOW_PWM_H

#include "pico/stdlib.h" /** Included for alarms, gpio functions, and typedefs */

/** \brief Opaque object defining a slow PWM output. */
typedef struct slow_pwm_s* slow_pwm;


/**
 * \brief Configure heater GPIO, reset duty cycle to 0, and start alarm callbacks.
 * 
 * \param pwm_pin GPIO pin number that the heater will attach to. Should be unused otherwise.
 * \param period_ms The period of the PWM wave in ms 
 * \param num_increments The number of increments between off and full on
 * 
 * \returns New slow_pwm object 
 */
slow_pwm slow_pwm_setup(uint8_t pwm_pin, uint period_ms, uint8_t num_increments);

/**
 * \brief Helper function to set the duty cycle of the slow PWM instance.
 * 
 * Updates the duty cycle of the given slow_pwm instance. New duty cycle will be used during
 * the next pwm cycle.
 * 
 * \param s Pointer to the slow_pwm struct that will be updated
 * \param duty New duty cycle. Clipped if greater than num_increments configured at setup.
 * 
 * \returns The new duty cycle.
 */
uint8_t slow_pwm_set_duty(slow_pwm s, uint8_t duty);

/**
 * \brief Helper function to set the duty cycle of the slow PWM instance using a float.
 * 
 * \param s Pointer to the slow_pwm struct that will be updated
 * \param u New duty cycle between 0 and 1.
 * 
 * \returns The new duty cycle.
 */
uint8_t slow_pwm_set_float_duty(slow_pwm s, float u);

/**
 * \brief Helper function to get the duty cycle of the slow PWM instance.
 * 
 * \param s Pointer to the slow_pwm object to read
 * 
 * \returns The current duty cycle.
 */
uint8_t slow_pwm_get_duty(slow_pwm s);

/**
 * \brief Free memory storing passed in slow_pwm object.
 * 
 * Sets GPIO low and disables both alarms.
 * 
 * \param s Slow PWM object to deinit
*/
void slow_pwm_deinit(slow_pwm s);

#endif
/** @} */