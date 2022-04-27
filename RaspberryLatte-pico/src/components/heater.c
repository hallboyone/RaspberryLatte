/**
 * @file heater.c
 * @author Richard Hall (hallboyone@icloud.com)
 * @brief Sets up a low frequnecy (~0.5Hz) PWm signal for controlling a heating element
 * through a zerocross SSR
 * @version 0.1
 * @date 2022-04-27
 * 
 */

/** 
 * Of course, we need the associated header file but we also need to include the uart bridge.
 */
#include "heater.h"
#include "uart_bridge.h"

/**
 * There are also a few internal variables that are needed for the heater to run correctly.
 * First, the pin the pwm is attached to needs to be saved. Second, the current duty cycle
 * also needs to be saved. Both of these will be accessed within timer callbacks and so are
 * declared volatile. 
 */
static volatile uint8_t _pwm_pin;
static volatile uint8_t _duty_cycle;
static struct repeating_timer _timer;

/** 
 * Of course, none of this will work without the timer callback. This should
 */
int64_t turn_off(alarm_id_t id, void *user_data) {
    gpio_put(_pwm_pin, 0);
    return 0;
}

bool turn_on(struct repeating_timer *t){
    if(_duty_cycle<99){ // if _duty_cycle == 99, don't bother turngin off
        add_alarm_in_ms(20*_duty_cycle, turn_off, NULL, true);
    }
    if(_duty_cycle>0){ // If _duty_cylce == 0, don't bother turning on
        gpio_put(_pwm_pin, 1);
    }
    return true;
}

void heater_setup(uint8_t pwm_pin){
    // Setup pwm pin as a digital output
    _pwm_pin = pwm_pin;
    gpio_init(_pwm_pin);
    gpio_set_dir(_pwm_pin, GPIO_OUT);

    _duty_cycle = 0;

    assignHandler(MSG_ID_SET_HEATER, &heater_set_duty_handler);

    add_repeating_timer_ms(-2000, turn_on, NULL, &_timer);
}

/**
 * This function is called to update a volitle variable containing the duty cycle. The value is
 * clipped between 0 (off) and 99 (full on). Since this function simply updates a variable used
 * within timer callback functions, if the timers have not been started with heater_setup this
 * function has no effect. In fact, there is no reasion this function should be called before
 * setting up the function. Instead of catching this error in this function (with an assert or
 * by simply doing nothing) I will just add a line to heter_setup that zeros out the duty cycle
 * when called. This way, if this function is called early its effects are reset before starting
 * the timers.
 */
void heater_set_duty(uint8_t duty_cycle);

/**
 * Finally, we conclude this component design with the uart handler. This function responds to
 * messages over uart with id MSG_ID_SET_HEATER that should have a 1 value payload containing 
 * the new duty cycle. Nothing is returned though perhaps a confermation message would be smart
 * to add in the future. 
 * 
 * The unconventional form of this function is dictated by the MessageHandler typedef in the 
 * uart_bridge header. 
 */
static void heater_set_duty_handler(int * data, int len);