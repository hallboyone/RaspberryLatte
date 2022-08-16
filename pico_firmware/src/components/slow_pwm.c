#include "slow_pwm.h"

/**
 * @brief Callback for alarm to turn off the heater pin
 * 
 * @param id ID of alarm triggering the callback (not used)
 * @param user_data Pointer to data used in the callback (not used)
 * @return int64_t New alarm interval or 0 if no alarm desired (always 0)
 */
int64_t _turn_off(alarm_id_t id, void *user_data) {
    slow_pwm * s = (slow_pwm*)user_data;
    gpio_put(s->_pwm_pin, 0);
    return 0;
}

/**
 * @brief Set the heater pin high and schedule the off alarm based on the current duty cycle
 * 
 * @param id ID of alarm triggering the callback (not used)
 * @param user_data Pointer to data used in the callback (not used)
 * @return int64_t New alarm after this many microseconds. Always PWM_PERIOD_MS*1000
 */
int64_t _start_period(alarm_id_t id, void *user_data){
    slow_pwm * s = (slow_pwm*)user_data;
    if(s->_duty_cycle < s->_num_increments-1){ // if _duty_cycle < number of increments, schedule off timer
        add_alarm_in_ms((s->_period_ms/s->_num_increments)*(s->_duty_cycle), _turn_off, s, true);
    }
    if(s->_duty_cycle > 0){ // If _duty_cylce > 0, turn on. 
        gpio_put(s->_pwm_pin, 1);
    }
    return s->_period_ms*1000;
}

void slow_pwm_setup(slow_pwm * s, uint8_t pwm_pin, uint period_ms, uint num_increments){
    s->_pwm_pin = pwm_pin;
    gpio_init(s->_pwm_pin);
    gpio_set_dir(s->_pwm_pin, GPIO_OUT);

    s->_duty_cycle = 0;
    s->_period_ms = period_ms;
    s->_num_increments = num_increments;
    add_alarm_in_ms(0, _start_period, s, true);
}

uint8_t slow_pwm_set_duty(slow_pwm * s, uint8_t duty){
    if (duty > s->_num_increments-1) duty = s->_num_increments-1;
    s->_duty_cycle = duty;
    return duty;
}

uint8_t slow_pwm_set_float_duty(slow_pwm * s, float u){
    return slow_pwm_set_duty(s, (s->_num_increments-1)*u);
}