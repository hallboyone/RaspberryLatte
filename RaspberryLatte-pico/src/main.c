#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/time.h"

#include "pinout.h"
#include "message_ids.h"

#include "maintainer.h"
#include "phasecontrol.h"
#include "binary_input.h"
#include "binary_output.h"
#include "pressure_sensor.h"
#include "uart_bridge.h"
#include "nau7802.h"
#include "lmt01.h"
#include "heater.h"
#include "solenoid.h"

#include "pid.h"

bool run = true;

void end_program(void * local_data, int * uart_data, int uart_data_len){
    run = false;
}

int main(){
    // Setup UART, clear queue, and assign endProgram command
    uart_bridge_setup();
    uart_bridge_register_handler(MSG_ID_END_PROGRAM, NULL, &end_program);

    pressure_sensor_setup(PRESSURE_SENSOR_PIN);

    const uint8_t pump_switch_gpio = PUMP_SWITCH_PIN;
    const uint8_t mode_select_gpio[2] = {DIAL_A_PIN, DIAL_B_PIN};
    binary_input_setup(1, &pump_switch_gpio, BINARY_INPUT_PULL_UP, true, false);
    binary_input_setup(2, mode_select_gpio, BINARY_INPUT_PULL_UP, false, true);
    
    const uint8_t led_pins[3] = {LED0_PIN, LED1_PIN, LED2_PIN};
    int leds_id = binary_output_setup(led_pins, 3);

    // Set up phase control
    const PhasecontrolConfig pump_config = 
    {.event          = ZEROCROSS_EVENT_RISING,
    .zerocross_pin   = PHASE_CONTROL_0CROSS_PIN,
    .out_pin         = PHASE_CONTROL_OUT_PIN,
    .zerocross_shift = PHASE_CONTROL_0CROSS_SHIFT};
    phasecontrol_setup(&pump_config);

    nau7802_setup(SCALE_CLOCK_PIN, SCALE_DATA_PIN, i2c1);

    heater_setup(HEATER_PWM_PIN);
    
    lmt01_setup(0, LMT01_DATA_PIN);

    solenoid_setup(SOLENOID_PIN);

    // Continually look for a messege and then run maintenance
    while(run){
        uint64_t start_time = time_us_64();
        readMessage();
        runMaintenance();
        uint64_t delta_time = time_us_64() - start_time;
        if(delta_time < 50){
            sleep_us(50 - delta_time);
        }
    }
}