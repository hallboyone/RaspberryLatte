/**
 * \file slow_pwm.h
 * \brief Sets up a low frequnecy (~0.5Hz) PWM signal
 * \author Richard Hall (hallboyone@icloud.com)
 *
 * Uses the alarm functions built into the rp2040 SDK. Useful in cases where fast switching
 * would be undesirable such as zerocross SSRs.
 */
#ifndef SLOW_PWM_H
#define SLOW_PWM_H

#include "pico/stdlib.h" /** Included for alarms, gpio functions, and typedefs */

/**
 * \brief Struct containing information for a single slow PWM instance.
 */
typedef struct {
    uint8_t _pwm_pin;     /**< The output pin number for the slow PWM. */
    uint8_t _duty_cycle;  /**< The current duty cycle for the slow PWM. */
    uint _period_ms;      /**< The period of the PWM wave in ms */
    uint _num_increments; /**< The number of increments between off and full on */
} slow_pwm;

/**
 * \brief Configure heater GPIO, reset duty cycle to 0, and start alarm callbacks.
 * 
 * \param s Pointer to the slow_pwm struct that will contain the required values for the pwm output.
 * \param pwm_pin GPIO pin number that the heater will attach to. Should be unused otherwise.
 * \param period_ms The period of the PWM wave in ms 
 * \param num_increments The number of increments between off and full on 
 */
void slow_pwm_setup(slow_pwm * s, uint8_t pwm_pin, uint period_ms, uint num_increments);

/**
 * \brief Helper function to set the duty cycle of the slow PWM instance.
 * 
 * Updates the duty cycle of the given slow_pwm instance. New duty cycle will be used during
 * the next pwm cycle.
 * 
 * \param s Pointer to the slow_pwm struct that will be updated
 * \param duty New duty cycle between 0 and 255.
 * 
 * \returns The new duty cycle.
 */
uint8_t slow_pwm_set_duty(slow_pwm * s, uint8_t duty);

/**
 * \brief Helper function to set the duty cycle of the slow PWM instance using a float.
 * 
 * \param s Pointer to the slow_pwm struct that will be updated
 * \param u New duty cycle between 0 and 1.
 * 
 * \returns The new duty cycle.
 */
uint8_t slow_pwm_set_float_duty(slow_pwm * s, float u);
#endif