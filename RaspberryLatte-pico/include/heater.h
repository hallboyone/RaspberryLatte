/**
 * @file heater.h
 * @author Richard Hall (hallboyone@icloud.com)
 * @brief Sets up a low frequnecy (~0.5Hz) PWm signal for controlling a heating element
 * through a zerocross SSR
 * @version 0.1
 * @date 2022-04-27
 * 
 */

/** 
 * We need to include the standard library for the pico SDK. At the very least, this will
 * be for the typedefs but it will also be used later for GPIO inits. The timer header will
 * be included in the source file to setup the low freq PWM callbacks.
 */
#include "pico/stdlib.h"

/**
 * Now that the typedefs are included, we can define the setup function. Like the others in 
 * this project, it uses the <file_name>_setup nameing convention. A single uint8_t (unsigned
 * char) is passed to it specifying the pwm output pin. This should not be used by any other
 * part of the software. The function will setup the pin as a digital output and start a
 * repeating timer callback that fires every 2 seconds. This callback checks if the the current
 * duty cycle (set with heater_set_duty) is nonzero. If it is, it turns the digital output
 * high and sets an alarm to turn it off after a timeout proportional to the duty cycle. If
 * the duty cycle is zero, however, then nothing happens. Note that the period of this process 
 * is 2 seconds. Therefore, changes to the duty cycle that are more frequent will not make any 
 * difference. 
 * 
 * In addition to setting up this callback, the setup function also resets the duty cylce to 0
 * (don't want to turn it on by accedent) and registers the uart handler. This cancels out the
 * danger of calling heater_set_duty before heater_setup and gets it ready to update the duty
 * cycle based on uart messages. 
 */
void heater_setup(uint8_t pwm_pin);

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