/**
 * \file slow_pwm.c
 * \ingroup slow_pwm
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Slow PWM source
 * \version 0.1
 * \date 2022-12-09
 */

#include "utils/slow_pwm.h"

#include <stdlib.h>

#include "utils/macros.h"

/**
 * \brief Struct containing information for a single slow PWM instance.
 */
typedef struct slow_pwm_s{
    uint8_t pwm_pin;     /**< The output pin number for the slow PWM. */
    uint8_t duty_cycle;  /**< The current duty cycle for the slow PWM. */
    uint period_ms;      /**< The period of the PWM wave in ms */
    uint num_increments; /**< The number of increments between off and full on */
    alarm_id_t off_alarm;
    alarm_id_t on_alarm;
} slow_pwm_;

/**
 * @brief Callback for alarm to turn off the heater pin
 * 
 * @param id ID of alarm triggering the callback (not used)
 * @param user_data Pointer to data used in the callback (not used)
 * @return int64_t New alarm interval or 0 if no alarm desired (always 0)
 */
static int64_t _turn_off(alarm_id_t id, void *user_data) {
    UNUSED_PARAMETER(id);
    slow_pwm s = (slow_pwm)user_data;
    gpio_put(s->pwm_pin, 0);
    return 0;
}

/**
 * @brief Set the heater pin high and schedule the off alarm based on the current duty cycle
 * 
 * @param id ID of alarm triggering the callback (not used)
 * @param user_data Pointer to data used in the callback (not used)
 * @return int64_t New alarm after this many microseconds. Always PWM_PERIOD_MS*1000
 */
static int64_t _start_period(alarm_id_t id, void *user_data){
    UNUSED_PARAMETER(id);
    slow_pwm s = (slow_pwm)user_data;
    if(s->duty_cycle < s->num_increments-1){ // if duty_cycle < number of increments, schedule off timer
        s->off_alarm = add_alarm_in_ms((s->period_ms/s->num_increments)*(s->duty_cycle), _turn_off, s, true);
    }
    if(s->duty_cycle > 0){ // If _duty_cycle > 0, turn on. 
        gpio_put(s->pwm_pin, 1);
    }
    return s->period_ms*1000;
}

slow_pwm slow_pwm_setup(uint8_t pwm_pin, uint period_ms, uint8_t num_increments){
    slow_pwm s = malloc(sizeof(slow_pwm_));

    s->pwm_pin = pwm_pin;
    gpio_init(s->pwm_pin);
    gpio_set_dir(s->pwm_pin, GPIO_OUT);

    s->duty_cycle = 0;
    s->period_ms = period_ms;
    s->num_increments = num_increments;
    s->on_alarm = add_alarm_in_ms(0, _start_period, s, true);

    return s;
}

uint8_t slow_pwm_set_duty(slow_pwm s, uint8_t duty){
    if (duty > s->num_increments-1) duty = s->num_increments-1;
    s->duty_cycle = duty;
    return duty;
}

uint8_t slow_pwm_set_float_duty(slow_pwm s, float u){
    if(u < 0.0) u = 0.0;
    else if (u > 1.0) u = 1.0;
    return slow_pwm_set_duty(s, (uint8_t)(u*(s->num_increments-1)));
}

uint8_t slow_pwm_get_duty(slow_pwm s){
    return s->duty_cycle;
}

void slow_pwm_deinit(slow_pwm s){
    cancel_alarm(s->on_alarm);
    gpio_put(s->pwm_pin, 0);
    cancel_alarm(s->off_alarm);
    free(s);
}