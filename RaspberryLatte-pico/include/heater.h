/**
 * @file heater.h
 * @author Richard Hall (hallboyone@icloud.com)
 * @brief Sets up a low frequnecy (~0.5Hz) PWm signal for controlling a heating element
 * through a zerocross SSR
 * @version 0.1
 * @date 2022-04-27
 * 
 */

#include "pico/stdlib.h" /** Included for alarms, gpio functions, and typedefs */

/**
 * @brief Configure heater GPIO, assign UART handler, reset duty cycle to 0, and start alarms.
 * 
 * @param pwm_pin GPIO pin number that the heater will attach to. Should be unused otherwise.
 */
void heater_setup(uint8_t pwm_pin);