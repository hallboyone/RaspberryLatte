#include <stdio.h>

#include "espresso_machine.h"
#include "loop_rate_limiter.h"

int main(){
    // Setup UART
    stdio_uart_init_full(PICO_DEFAULT_UART_INSTANCE, 115200, PICO_DEFAULT_UART_TX_PIN, PICO_DEFAULT_UART_RX_PIN);

    // Setup the pressure sensor
    analog_input_setup(&pressure_sensor, PRESSURE_SENSOR_PIN);

    // Setup the LED binary output
    const uint8_t led_pins[3] = {LED0_PIN, LED1_PIN, LED2_PIN};
    binary_output_setup(&leds, led_pins, 3);

    // Setup the binary inputs for pump switch and mode dial.
    const uint8_t pump_switch_gpio = PUMP_SWITCH_PIN;
    const uint8_t mode_select_gpio[2] = {DIAL_A_PIN, DIAL_B_PIN};
    binary_input_setup(&pump_switch, 1, &pump_switch_gpio, BINARY_INPUT_PULL_UP, true, false);
    binary_input_setup(&mode_dial, 2, mode_select_gpio, BINARY_INPUT_PULL_UP, false, true);

    // Setup phase control
    phasecontrol_setup(&pump,PHASECONTROL_0CROSS_PIN,PHASECONTROL_OUT_PIN,PHASECONTROL_0CROSS_SHIFT,ZEROCROSS_EVENT_RISING);

    // Setup solenoid as a binary output
    uint8_t solenoid_pin [1] = {SOLENOID_PIN};
    binary_output_setup(&solenoid, solenoid_pin, 1);

    // Setup nau7802. This is the only non-struct based object. 
    nau7802_setup(SCALE_CLOCK_PIN, SCALE_DATA_PIN, i2c1);
    scale_zero();

    // Setup heater as a slow_pwm object
    slow_pwm_setup(&heater, HEATER_PWM_PIN);
    heater_pid.K.p = 0.05; heater_pid.K.i = 0.0015; heater_pid.K.d = 0.0005;
    heater_pid.min_time_between_ticks_ms = 100;
    heater_pid.sensor = &read_boiler_thermo;
    heater_pid.plant = &apply_boiler_input;
    update_setpoint();
    pid_init(&heater_pid, 0, 150, 1000);

    // Setup thermometer
    lmt01_setup(&thermo, 0, LMT01_DATA_PIN);

    autobrew_leg_setup_function_call(&(autobrew_legs[0]), 0, &scale_zero);
    autobrew_leg_setup_linear_power(&(autobrew_legs[1]),  60,  80,  4000000, NULL);
    autobrew_leg_setup_linear_power(&(autobrew_legs[2]),   0,   0,  4000000, NULL);
    autobrew_leg_setup_linear_power(&(autobrew_legs[3]),  60, 127,  1000000, NULL);
    autobrew_leg_setup_linear_power(&(autobrew_legs[4]), 127, 127, 60000000, &scale_at_output);
    autobrew_routine_setup(&autobrew_plan, autobrew_legs, 5);

    // Run main machine loop
    uint loop_counter = 0;
    uint64_t last_loop_time_us = 0;
    while(true){
        loop_rate_limiter_us(&last_loop_time_us, 10000);

        update_setpoint();
        pid_tick(&heater_pid);
        update_pump();
        update_leds();

        loop_counter = (loop_counter+1)%100;
        if(loop_counter==0){
            printf("Setpoint: %0.2f, Temp: %0.4f\n", heater_pid.setpoint, lmt01_read(&thermo)/16.0);
        }
    }
}