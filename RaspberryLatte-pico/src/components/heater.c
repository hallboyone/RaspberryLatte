/**
 * @file heater.c
 * @author Richard Hall (hallboyone@icloud.com)
 * @brief Sets up a low frequnecy (~1Hz) PWM signal for controlling a heating element
 * through a zerocross SSR
 * @version 0.1
 * @date 2022-04-27
 * 
 */

#include "heater.h"         /** Function definitions */
#include "uart_bridge.h"    /** Message handling */
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
int64_t turn_off(alarm_id_t id, void *user_data) {
    gpio_put(_pwm_pin, 0);
    return 0;
}

/**
 * @brief Set the heater pin high and schedule the off alarm based on the current duty cycle
 * 
 * @param id ID of alarm triggering the callback (not used)
 * @param user_data Pointer to data used in the callback (not used)
 * @return int64_t New alarm after this many microseconds. Always PWM_PERIOD_MS*1000
 */
int64_t turn_on(alarm_id_t id, void *user_data){
    if(_duty_cycle < PWM_INCREMENTS-1){ // if _duty_cycle < 63, schedule off timer
        add_alarm_in_ms((PWM_PERIOD_MS/PWM_INCREMENTS)*_duty_cycle, turn_off, NULL, true);
    }
    if(_duty_cycle > 0){ // If _duty_cylce > 0, turn on. 
        gpio_put(_pwm_pin, 1);
    }
    return PWM_PERIOD_MS*1000;
}

/**
 * @brief Updates the internal duty cycle variable whenever a UART message is received with the ID MSG_ID_SET_HEATER
 * 
 * @param data Should be a single integer array with the new duty cycle. Values out of range are clipped. 
 * @param len Must be greater than or equal to 1. If less than one, nothing happens. 
 */
static void heater_set_duty_handler(int * data, int len){
    if(len==1){
        _duty_cycle = *data;
        if(_duty_cycle < 0){
            _duty_cycle = 0;
        } else if(_duty_cycle> PWM_INCREMENTS-1){
            _duty_cycle = PWM_INCREMENTS-1;
        }
        sendMessageWithStatus(MSG_ID_SET_HEATER, SUCCESS, &_duty_cycle, 1);
    } else {
        _duty_cycle = 0;
        sendMessageWithStatus(MSG_ID_SET_HEATER, MSG_FORMAT_ERROR, &_duty_cycle, 1);
    }
}

/**
 * @brief Configure heater GPIO, assign UART handler, reset duty cycle to 0, and start alarms.
 * 
 * @param pwm_pin GPIO pin number that the heater will attach to. Should be unused otherwise.
 */
void heater_setup(uint8_t pwm_pin){
    _pwm_pin = pwm_pin;
    gpio_init(_pwm_pin);
    gpio_set_dir(_pwm_pin, GPIO_OUT);

    _duty_cycle = 0;

    registerHandler(MSG_ID_SET_HEATER, &heater_set_duty_handler);

    add_alarm_in_ms(0, turn_on, NULL, true);
}