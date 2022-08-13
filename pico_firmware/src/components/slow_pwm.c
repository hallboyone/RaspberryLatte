/**
 * \file slow_pwm.c
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Sets up a low frequnecy (~0.5Hz) PWM signal
 */

#include "slow_pwm.h"         /** Function definitions */
#include "status_ids.h"

#define PWM_PERIOD_MS 1050 /** Length of 1 PWM cycle. Set to (1000/60)*63 */
#define PWM_INCREMENTS  64 /** Number of discrete PWM settings Valid setting will be [0,PWM_INCREMENTS-1]*/
/**
 * There are also a few internal variables that are needed for the heater to run correctly.
 * First, the pin the pwm is attached to needs to be saved. Second, the current duty cycle
 * also needs to be saved. Both of these will be accessed within timer callbacks and so are
 * declared volatile. 
 */
static volatile uint8_t _pwm_pin;
static volatile uint8_t _duty_cycle;

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
int64_t _turn_on(alarm_id_t id, void *user_data){
    slow_pwm * s = (slow_pwm*)user_data;
    if(s->_duty_cycle < PWM_INCREMENTS-1){ // if _duty_cycle < 63, schedule off timer
        add_alarm_in_ms((PWM_PERIOD_MS/PWM_INCREMENTS)*(s->_duty_cycle), _turn_off, s, true);
    }
    if(s->_duty_cycle > 0){ // If _duty_cylce > 0, turn on. 
        gpio_put(s->_pwm_pin, 1);
    }
    return PWM_PERIOD_MS*1000;
}

void slow_pwm_setup(slow_pwm * s, uint8_t pwm_pin){
    s->_pwm_pin = pwm_pin;
    gpio_init(s->_pwm_pin);
    gpio_set_dir(s->_pwm_pin, GPIO_OUT);

    s->_duty_cycle = 0;

    add_alarm_in_ms(0, _turn_on, s, true);
}

uint8_t slow_pwm_set_duty(slow_pwm * s, uint8_t duty){
    if (duty > PWM_INCREMENTS-1) duty = PWM_INCREMENTS-1;
    s->_duty_cycle = duty;
    return duty;
}

uint8_t slow_pwm_set_float_duty(slow_pwm * s, float u){
    if (u > 1) u = 1;
    else if (u < 0) u = 0;
    return slow_pwm_set_duty(s, (PWM_INCREMENTS-1)*u);
}

void slow_pwm_set_uart_callback(message_id id, void * local_data, int * uart_data, int uart_data_len){
    slow_pwm * s = (slow_pwm*)local_data;
    if(uart_data_len==1){
        int tmp_val = slow_pwm_set_duty(s, uart_data[0]);
        sendMessageWithStatus(id, SUCCESS, &tmp_val, 1);
    } else {
        int tmp_val = slow_pwm_set_duty(s, 0);
        sendMessageWithStatus(id, MSG_FORMAT_ERROR, &tmp_val, 1);
    }
}
