#include "pinout.h"

#include "analog_input.h"
#include "binary_output.h"
#include "binary_input.h"
#include "phasecontrol.h"
#include "nau7802.h"
#include "slow_pwm.h"
#include "lmt01.h"

#include "pid.h"
#include "autobrew.h"

#include "espresso_machine.h"
#include "brew_parameters.h"

static espresso_machine_state _state = {.pump.pump_lock = true}; 

static analog_input  pressure_sensor;
static binary_output leds;
static binary_input  pump_switch, mode_dial;
static phasecontrol  pump;
static binary_output solenoid;
static slow_pwm      heater;
static pid_ctrl      heater_pid;
static lmt01         thermo; 

static autobrew_leg autobrew_legs [5];
static autobrew_routine autobrew_plan;

/**
 * \brief Helper function for the PID controller. Returns the boiler temp in C.
 */
static float read_boiler_thermo(){
    return lmt01_read_float(&thermo);
}

/**
 * \brief Helper function for the PID controller. Applies an input to the boiler heater.
 */
static void apply_boiler_input(float u){
    slow_pwm_set_float_duty(&heater, u);
}

/** 
 * \brief Returns true if scale is greater than or equal to the current output. 
 */
static bool scale_at_output(){
    return nau7802_at_val_mg(BREW_YIELD_MG);
}

/**
 * \brief Uses the switch state to update the machine's configuration.
 */
static void espresso_machine_update_state(){
    // Switches
    _state.switches.ac_switch = phasecontrol_is_ac_hot(&pump);
    _state.switches.pump_switch = binary_input_read(&pump_switch);

    // Dial
    uint8_t new_mode = binary_input_read(&mode_dial);
    bool mode_changed = (new_mode != _state.switches.mode_dial);
    _state.switches.mode_dial = new_mode;

    //Pump lock
    _state.pump.pump_lock = _state.switches.pump_switch && (mode_changed || _state.pump.pump_lock);

    // Zero scale if mode changed
    if(mode_changed) nau7802_zero();

    // Update setpoints
    if(_state.switches.ac_switch) _state.boiler.setpoint = 16*TEMP_SETPOINTS[new_mode];
    else _state.boiler.setpoint = 0;
}

static void espresso_machine_update_pump(){
    if(!_state.switches.pump_switch){
        autobrew_routine_reset(&autobrew_plan);
        phasecontrol_set_duty_cycle(&pump, 0);
        binary_output_put(&solenoid, 0, 0);
    } else if (_state.pump.pump_lock){
        phasecontrol_set_duty_cycle(&pump, 0);
        binary_output_put(&solenoid, 0, 0);
    } else {
        switch(_state.switches.mode_dial){
            case MODE_STEAM:
                phasecontrol_set_duty_cycle(&pump, 0);
                binary_output_put(&solenoid, 0, 0);
                break;
            case MODE_HOT:
                phasecontrol_set_duty_cycle(&pump, 127);
                binary_output_put(&solenoid, 0, 0);
                break;
            case MODE_MANUAL:
                phasecontrol_set_duty_cycle(&pump, 127);
                binary_output_put(&solenoid, 0, 1);
                break;
            case MODE_AUTO:
                if(!autobrew_routine_tick(&autobrew_plan)){
                    binary_output_put(&solenoid, 0, 1);
                    if(autobrew_plan.state.pump_setting_changed){
                        phasecontrol_set_duty_cycle(&pump, autobrew_plan.state.pump_setting);
                    }
                } else {
                    phasecontrol_set_duty_cycle(&pump, 0);
                    binary_output_put(&solenoid, 0, 0);
                }
                break;
        }
    }
}

static void espresso_machine_update_boiler(){
    heater_pid.setpoint = _state.boiler.setpoint/16.;
    pid_tick(&heater_pid);

    _state.boiler.tempurature = lmt01_read(&thermo);
}

static void espresso_machine_update_leds(){
    binary_output_put(&leds, 0, _state.switches.ac_switch);
    binary_output_put(&leds, 1, _state.switches.ac_switch && lmt01_read_float(&thermo) - heater_pid.setpoint < 2.5 && lmt01_read_float(&thermo) - heater_pid.setpoint > -2.5);
    binary_output_put(&leds, 2, _state.switches.ac_switch && !_state.switches.pump_switch && nau7802_at_val_mg(BREW_DOSE_MG));
}

int espresso_machine_setup(espresso_machine_viewer * state_viewer){
    if(state_viewer != NULL){
        *state_viewer = &_state;
    }

    // Setup the pressure sensor
    if(analog_input_setup(&pressure_sensor, PRESSURE_SENSOR_PIN)){
        return 1;
    }

    // Setup the LED binary output
    const uint8_t led_pins[3] = {LED0_PIN, LED1_PIN, LED2_PIN};
    binary_output_setup(&leds, led_pins, 3);

    // Setup the binary inputs for pump switch and mode dial.
    const uint8_t pump_switch_gpio = PUMP_SWITCH_PIN;
    const uint8_t mode_select_gpio[2] = {DIAL_A_PIN, DIAL_B_PIN};
    binary_input_setup(&pump_switch, 1, &pump_switch_gpio, BINARY_INPUT_PULL_UP, true, false);
    inary_input_setup(&mode_dial, 2, mode_select_gpio, BINARY_INPUT_PULL_UP, false, true);

    // Setup phase control
    phasecontrol_setup(&pump,PHASECONTROL_0CROSS_PIN,PHASECONTROL_OUT_PIN,PHASECONTROL_0CROSS_SHIFT,ZEROCROSS_EVENT_RISING);

    // Setup solenoid as a binary output
    uint8_t solenoid_pin [1] = {SOLENOID_PIN};
    binary_output_setup(&solenoid, solenoid_pin, 1);

    // Setup nau7802. This is the only non-struct based object. 
    nau7802_setup(SCALE_CLOCK_PIN, SCALE_DATA_PIN, i2c1, SCALE_CONVERSION_MG);

    // Setup heater as a slow_pwm object
    slow_pwm_setup(&heater, HEATER_PWM_PIN);
    heater_pid.K.p = PID_GAIN_P; heater_pid.K.i = PID_GAIN_I; heater_pid.K.d = PID_GAIN_D;
    heater_pid.min_time_between_ticks_ms = 100;
    heater_pid.sensor = &read_boiler_thermo;
    heater_pid.plant = &apply_boiler_input;
    heater_pid.setpoint = 0;
    pid_init(&heater_pid, 0, 150, 1000);

    // Setup thermometer
    lmt01_setup(&thermo, 0, LMT01_DATA_PIN);

    autobrew_leg_setup_function_call(&(autobrew_legs[0]), 0, &nau7802_zero);
    autobrew_leg_setup_linear_power(&(autobrew_legs[1]),  60,  AUTOBREW_PREINFUSE_END_POWER,  AUTOBREW_PREINFUSE_ON_TIME_US, NULL);
    autobrew_leg_setup_linear_power(&(autobrew_legs[2]),   0,   0,  AUTOBREW_PREINFUSE_OFF_TIME_US, NULL);
    autobrew_leg_setup_linear_power(&(autobrew_legs[3]),  60, 127,  AUTOBREW_BREW_RAMP_TIME, NULL);
    autobrew_leg_setup_linear_power(&(autobrew_legs[4]), 127, 127, AUTOBREW_BREW_TIMEOUT_US, &scale_at_output);
    autobrew_routine_setup(&autobrew_plan, autobrew_legs, 5);

    espresso_machine_update_state();

    return 0;
}

void espresso_machine_tick(){
    espresso_machine_update_state();
    espresso_machine_update_boiler();
    espresso_machine_update_pump();
    espresso_machine_update_leds();
}