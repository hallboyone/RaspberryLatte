#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/time.h"

#include "machine_configuration.h"
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

#include "pid.h"
#include "autobrew.h"

analog_input pressure_sensor;
binary_output leds;
binary_input pump_switch, mode_dial;
phasecontrol pump;
binary_output solenoid;
slow_pwm heater;
pid_ctrl heater_pid;
lmt01 thermo;

autobrew_leg autobrew_legs [5];
autobrew_routine autobrew_plan;

int32_t scale_origin = 0;

bool pump_lock = false;

static float read_boiler_thermo(){
    return lmt01_read_float(&thermo);
}

static void apply_boiler_input(float u){
    slow_pwm_set_float_duty(&heater, u);
}

static int read_scale(){
    int32_t scale_val;
    nau7802_read(&scale_val);
    return 0.152710615479*(float)(scale_val - scale_origin); // in mg
}

static void zero_scale(){
    do {
        nau7802_read(&scale_origin);
    } while (scale_origin==0);
}

static bool scale_at_dose(){
    return read_scale() > 1000 * 30;
}

static uint64_t last_loop_time_us = 0;
void loop_rate_limiter_us(const uint64_t loop_period_us){
    if(last_loop_time_us==0){
        last_loop_time_us = time_us_64();
    }
    else{
        while(last_loop_time_us + loop_period_us > time_us_64()){
            tight_loop_contents();
        }
        last_loop_time_us = time_us_64();
    }
}

static inline void update_setpoint(){
    if(false && phasecontrol_is_ac_hot(&pump)){
        switch(binary_input_read(&mode_dial)){
            case MODE_STEAM:
                heater_pid.setpoint = SETPOINT_STEAM;
                break;
            case MODE_HOT:
                heater_pid.setpoint = SETPOINT_HOT;
                break;
            default:
                heater_pid.setpoint = SETPOINT_BREW;
        }
    } else {
        heater_pid.setpoint = 0;
    }
}

static inline void update_pump(){
    if(pump_lock || !binary_input_read(&pump_switch)){
        phasecontrol_set_duty_cycle(&pump, 0);
        binary_output_put(&solenoid, 0, 0);
        autobrew_routine_reset(&autobrew_plan);
    } else if (binary_input_read(&pump_switch)){
        switch(binary_input_read(&mode_dial)){
            case MODE_STEAM:
                phasecontrol_set_duty_cycle(&pump, 0);
                binary_output_put(&solenoid, 0, 0);
                break;
            case MODE_HOT:
                phasecontrol_set_duty_cycle(&pump, 127);
                binary_output_put(&solenoid, 0, 0);
                break;
            case MODE_MAN:
                phasecontrol_set_duty_cycle(&pump, 127);
                binary_output_put(&solenoid, 0, 1);
                break;
            case MODE_AUTO:
                autobrew_routine_tick(&autobrew_plan);
                if(!autobrew_plan.state.finished){
                    if(autobrew_plan.state.pump_setting_changed){
                        phasecontrol_set_duty_cycle(&pump, autobrew_plan.state.pump_setting);
                    }
                    binary_output_put(&solenoid, 0, 1);
                } else {
                    phasecontrol_set_duty_cycle(&pump, 0);
                    binary_output_put(&solenoid, 0, 0);
                }
                break;
        }
    }
}

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

    autobrew_leg_setup_function_call(&(autobrew_legs[0]), 0, &zero_scale);
    autobrew_leg_setup_linear_power(&(autobrew_legs[1]),  60,  80,  4000000, NULL);
    autobrew_leg_setup_linear_power(&(autobrew_legs[2]),   0,   0,  4000000, NULL);
    autobrew_leg_setup_linear_power(&(autobrew_legs[3]),  60, 127,  1000000, NULL);
    autobrew_leg_setup_linear_power(&(autobrew_legs[4]), 127, 127, 60000000, &scale_at_dose);
    autobrew_routine_setup(&autobrew_plan, autobrew_legs, 5);

    // Run main machine loop
    uint loop_counter = 0;
    zero_scale();
    while(true){
        loop_rate_limiter_us(50000);
        binary_output_put(&leds, 0, phasecontrol_is_ac_hot(&pump));

        update_setpoint();
        pid_tick(&heater_pid);

        update_pump();

        loop_counter = (loop_counter+1)%20;
        
        if(loop_counter==0){
            printf("Scale: %0.2f, At dose: %d\n", read_scale()/1000.0, scale_at_dose());
            printf("Setpoint: %0.2f, Temp: %0.4f\n", heater_pid.setpoint, lmt01_read(&thermo)/16.0);
        }
    }
}