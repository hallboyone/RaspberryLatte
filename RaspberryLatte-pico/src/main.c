#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/time.h"

#include "pinout.h"
#include "message_ids.h"

#include "maintainer.h"
#include "uart_bridge.h"

#include "analog_input.h"
#include "binary_output.h"
#include "binary_input.h"
#include "phasecontrol.h"
#include "nau7802.h"
#include "slow_pwm.h"
#include "lmt01.h"

// #include "pid.h"

bool run = true;

void end_program(message_id id, void * local_data, int * uart_data, int uart_data_len){
    run = false;
}

int main(){
    // Setup UART and assign endProgram command
    uart_bridge_setup();
    uart_bridge_register_handler(MSG_ID_END_PROGRAM, NULL, &end_program);

    // Define a pressure sensor, set it up, and register its callback
    analog_input pressure_sensor;
    analog_input_setup(&pressure_sensor, PRESSURE_SENSOR_PIN);
    uart_bridge_register_handler(MSG_ID_GET_PRESSURE, &pressure_sensor, &analog_input_uart_callback);

    // Define LED binary output, set it up, and register its callback
    binary_output leds;
    const uint8_t led_pins[3] = {LED0_PIN, LED1_PIN, LED2_PIN};
    binary_output_setup(&leds, led_pins, 3);
    uart_bridge_register_handler(MSG_ID_SET_LEDS, &leds, &binary_output_uart_callback);

    // Define binary inputs for pump switch and mode dial. Setup and register their callbacks.
    binary_input pump_switch, mode_dial;
    const uint8_t pump_switch_gpio = PUMP_SWITCH_PIN;
    const uint8_t mode_select_gpio[2] = {DIAL_A_PIN, DIAL_B_PIN};
    binary_input_setup(&pump_switch, 1, &pump_switch_gpio, BINARY_INPUT_PULL_UP, true, false);
    binary_input_setup(&mode_dial, 2, mode_select_gpio, BINARY_INPUT_PULL_UP, false, true);
    uart_bridge_register_handler(MSG_ID_GET_SWITCH, &pump_switch, &binary_input_uart_callback);
    uart_bridge_register_handler(MSG_ID_GET_DIAL, &mode_dial, &binary_input_uart_callback);

    // Set up phase control
    phasecontrol pump;
    phasecontrol_setup(&pump,PHASE_CONTROL_0CROSS_PIN,PHASE_CONTROL_OUT_PIN,PHASE_CONTROL_0CROSS_SHIFT,ZEROCROSS_EVENT_RISING);
    uart_bridge_register_handler(MSG_ID_GET_AC_ON, &pump, &phasecontrol_is_ac_hot_uart_callback);
    uart_bridge_register_handler(MSG_ID_SET_PUMP, &pump, phasecontrol_set_duty_uart_callback);

    // Setup solenoid as a binary output
    binary_output solenoid;
    uint8_t solenoid_pin [1] = {SOLENOID_PIN};
    binary_output_setup(&solenoid, solenoid_pin, 1);
    uart_bridge_register_handler(MSG_ID_SET_SOLENOID, &solenoid, &binary_output_uart_callback);

    // Setup nau7802. This is the only non-struct based object. 
    nau7802_setup(SCALE_CLOCK_PIN, SCALE_DATA_PIN, i2c1);
    uart_bridge_register_handler(MSG_ID_GET_WEIGHT, NULL, &nau7802_read_uart_callback);

    // Setup heater and register its handler.
    slow_pwm heater;
    slow_pwm_setup(&heater, HEATER_PWM_PIN);
    uart_bridge_register_handler(MSG_ID_SET_HEATER, &heater, &slow_pwm_set_uart_callback);
    
    // Setup thermometer and register its handler.
    lmt01 thermo;
    lmt01_setup(&thermo, 0, LMT01_DATA_PIN);
    uart_bridge_register_handler(MSG_ID_GET_TEMP, &thermo, &lmt01_read_uart_callback);

    // Continually look for a messege and then run maintenance
    while(run){
        uint64_t start_time = time_us_64();
        readMessage();
        uint64_t delta_time = time_us_64() - start_time;
        if(delta_time < 50){
            sleep_us(50 - delta_time);
        }
    }
}