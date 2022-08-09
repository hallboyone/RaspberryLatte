/**
 * \file slow_pwm.h
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Sets up a low frequnecy (~0.5Hz) PWM signal
 */
#ifndef _SLOW_PWM_H
#define _SLOW_PWM_H

#include "pico/stdlib.h" /** Included for alarms, gpio functions, and typedefs */

#include "uart_bridge.h"    /** Message handling */

typedef struct {
    uint8_t _pwm_pin;
    uint8_t _duty_cycle;
} slow_pwm;

/**
 * \brief Configure heater GPIO, assign UART handler, reset duty cycle to 0, and start alarms.
 * 
 * \param pwm_pin GPIO pin number that the heater will attach to. Should be unused otherwise.
 */
void slow_pwm_setup(slow_pwm * s, uint8_t pwm_pin);

uint8_t slow_pwm_set_duty(slow_pwm * s, uint8_t duty);

uint8_t slow_pwm_set_float_duty(slow_pwm * s, float u);

void slow_pwm_set_uart_callback(message_id id, void * local_data, int * uart_data, int uart_data_len);
#endif