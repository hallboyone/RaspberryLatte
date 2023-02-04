/**
 * \file espresso_machine.c
 * \ingroup espresso_machine
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Espresso Machine source
 * \version 0.1
 * \date 2022-12-09
 */

#include "machine_logic/espresso_machine.h"

#include <stdio.h>

#include "pinout.h"

#include "machine_logic/autobrew.h"
#include "machine_logic/machine_settings.h"


#include "drivers/nau7802.h"
#include "drivers/lmt01.h"
#include "drivers/mb85_fram.h"
#include "drivers/flow_meter.h"
#include "drivers/ulka_pump.h"

#include "utils/gpio_irq_timestamp.h"
#include "utils/analog_input.h"
#include "utils/binary_output.h"
#include "utils/binary_input.h"
#include "utils/slow_pwm.h"
#include "utils/i2c_bus.h"
#include "utils/pid.h"

const float PID_GAIN_P = 0.05;
const float PID_GAIN_I = 0.00175;
const float PID_GAIN_D = 0.0005;
const float PID_GAIN_F = 0.05;

const float SCALE_CONVERSION_MG = -0.152710615479;
const float FLOW_CONVERSION_ML = 0.5;
const float PRESSURE_CONVERSION_BAR = 1.0;

static espresso_machine_state _state = {.pump.pump_lock = true}; 

/** I2C bus connected to RTC, memory, scale ADC, and I2C port */
static i2c_inst_t *  bus = i2c1;

/** All the peripheral components for the espresso machine */
static analog_input        pressure_sensor;
static binary_output       leds;
static binary_input        pump_switch, mode_dial;
static binary_output       solenoid;
static slow_pwm            heater;
static lmt01               thermo; 
static nau7802             scale;
static discrete_derivative scale_flowrate;
static mb85_fram           mem;
static flow_meter          flow;
static ulka_pump           pump;

static const machine_settings*   settings;

/** Autobrew and control objects */
static pid_ctrl         heater_pid;
static autobrew_routine autobrew_plan;

/**
 * \brief Helper function for the PID controller. Returns the boiler temp in C.
 */
static float read_boiler_thermo(){
    return lmt01_read_float(&thermo);
}

/**
 * \brief Helper function that tracks and returns the scale's flowrate
*/
static float read_pump_flowrate(){
    return flow_meter_rate(&flow);
}

/**
 * \brief Helper function for the PID controller. Applies an input to the boiler heater.
 * 
 * \param u Output for the boiler. 1 is full on, 0 is full off.
 */
static void apply_boiler_input(float u){
    slow_pwm_set_float_duty(&heater, u);
}

/** 
 * \brief Returns true if scale is greater than or equal to the current output. 
 */
static bool scale_at_output(){
    return nau7802_at_val_mg(&scale, *settings->brew.yield*100);
}

/**
 * \brief Helper function to zero the scale. Used by the autobrew routine. 
 */
static int zero_scale(){
    return nau7802_zero(&scale);
}

/**
 * \brief Checks if AC is hot
 * 
 * Looks at the last zerocross time. If not 'too' long ago, then AC is on and function returns true.
 * 
 * \return true AC is on
 * \return false AC is off
 */
static inline bool is_ac_on(){
    return (gpio_irq_timestamp_read_duration_us(AC_0CROSS_PIN) < 17000);
}

/**
 * \brief Setup the autobrew routine using the latest machine settings.
 * 
 * The way that the autobrew library is structured prevents direct setting adjustment. Whenever
 * changes are made, the routine must be remade. This is a small amount of overhead since changes
 * can only occur when the machine is off so this function only has to be called when it is switched
 * on. 
 */
static void espresso_machine_autobrew_setup(){
    uint32_t preinf_ramp_dur;
    uint32_t preinf_on_dur;
    uint8_t preinf_pwr = *settings->autobrew.preinf_power;
    uint8_t preinf_pwr_start = (preinf_pwr == 0) ? 0 : 60;
    if(*settings->autobrew.preinf_ramp_time <= *settings->autobrew.preinf_on_time){
        // Ramp and then run for remaining time
        preinf_ramp_dur = *settings->autobrew.preinf_ramp_time*100000UL;
        preinf_on_dur = (*settings->autobrew.preinf_on_time-(*settings->autobrew.preinf_ramp_time))*100000UL;
    } else {
        // Ramp for the full time
        preinf_ramp_dur = *settings->autobrew.preinf_on_time*100000UL;
        preinf_on_dur = 0;
    }

    uint32_t brew_ramp_dur;
    uint32_t brew_on_dur;
    uint8_t brew_pwr = *settings->brew.power;
    if(*settings->autobrew.preinf_ramp_time <= *settings->autobrew.timeout){
        // Ramp and then run for remaining time
        brew_ramp_dur = *settings->autobrew.preinf_ramp_time*100000UL;
        brew_on_dur = (*settings->autobrew.timeout*10-(*settings->autobrew.preinf_ramp_time))*100000UL;
    } else {
        // Ramp for the full time
        brew_ramp_dur = *settings->autobrew.timeout*1000000UL;
        brew_on_dur = 0;
    }

    uint32_t preinf_off_time = *settings->autobrew.preinf_off_time*100000UL;

    autobrew_setup_linear_power_leg(&autobrew_plan, 1, preinf_pwr_start, preinf_pwr, preinf_ramp_dur, NULL);
    autobrew_setup_linear_power_leg(&autobrew_plan, 2, preinf_pwr,       preinf_pwr, preinf_on_dur,   NULL);
    autobrew_setup_linear_power_leg(&autobrew_plan, 3, 0,                0,          preinf_off_time, NULL);
    autobrew_setup_linear_power_leg(&autobrew_plan, 4, 60,               brew_pwr,   brew_ramp_dur,   &scale_at_output);
    autobrew_setup_linear_power_leg(&autobrew_plan, 5, brew_pwr,         brew_pwr,   brew_on_dur,     &scale_at_output);
}

/**
 * \brief Flag changes in switch values. Run simple event-driven routines (zero scale, setup autobrew,
 * etc).
 */
static void espresso_machine_update_switches(){
    const bool new_ac_switch = is_ac_on();
    if(_state.switches.ac_switch != new_ac_switch){
        // If machine has been turned on or off
        _state.switches.ac_switch_changed = (_state.switches.ac_switch ? -1 : 1);
        _state.switches.ac_switch = new_ac_switch;

        // If machine has been switched on, rebuild autobrew routine
        if(new_ac_switch) espresso_machine_autobrew_setup();
    } else {
        _state.switches.ac_switch_changed = 0;
    }

    const bool new_pump_switch = binary_input_read(&pump_switch);
    if(_state.switches.pump_switch != new_pump_switch){
        _state.switches.pump_switch_changed = (_state.switches.pump_switch ? -1 : 1);
        _state.switches.pump_switch = new_pump_switch;
    } else {
        _state.switches.pump_switch_changed = 0;
    }

    const int new_mode_switch = binary_input_read(&mode_dial);
    if(_state.switches.mode_dial != new_mode_switch){
        _state.switches.mode_dial_changed = (_state.switches.mode_dial > new_mode_switch ? -1 : 1);
        _state.switches.mode_dial = new_mode_switch;
        nau7802_zero(&scale);
    } else {
        _state.switches.mode_dial_changed = 0;
    }
}

/**
 * \brief Handle any changes to the machine settings. The occurs when the AC is off and the pump 
 * switch is toggled. When AC is switched on, this function applies the latest settings and returns
 * the setting tree to root.
 */
static void espresso_machine_update_settings(){
    bool reset_settings_ui = (_state.switches.ac_switch_changed == 1);
    bool select_settings_ui = !_state.switches.ac_switch && _state.switches.pump_switch_changed;
    machine_settings_update(reset_settings_ui, select_settings_ui, _state.switches.mode_dial);
}

/** \brief Uses the state of the switches to update the pump and solenoid.
 * 
 * First, the pump lock is updated. This prevents the pump from coming on unintentionally (e.g. pump
 * switch on when turning AC switch on). Next the pump's power solenoid's state are updated based on
 * the current machine mode. Finally, the state of the pump is saved into the machine_state var to
 * facilitate its easy access.
*/
static void espresso_machine_update_pump(){
    // Lock the pump if AC is off OR pump is on and the mode has changed or it has been locked already.
    if(!_state.switches.ac_switch 
       || (_state.switches.pump_switch && (_state.switches.mode_dial_changed || ulka_pump_is_locked(&pump)))){
        ulka_pump_lock(&pump);
    } else {
        ulka_pump_unlock(&pump);
    }

    if(!_state.switches.pump_switch 
        || ulka_pump_is_locked(&pump) 
        || MODE_STEAM == _state.switches.mode_dial){
        // If the pump is locked, switched off, or in steam mode
        autobrew_routine_reset(&autobrew_plan);
        ulka_pump_off(&pump);
        binary_output_put(&solenoid, 0, 0);

    } else if (MODE_HOT == _state.switches.mode_dial){
        ulka_pump_pwr_percent(&pump, *settings->hot.power);
        binary_output_put(&solenoid, 0, 0);

    } else if (MODE_MANUAL == _state.switches.mode_dial){
        ulka_pump_pwr_percent(&pump, *settings->brew.power);
        binary_output_put(&solenoid, 0, 1);

    } else if (MODE_AUTO ==_state.switches.mode_dial){
        if(!autobrew_routine_tick(&autobrew_plan)){
            binary_output_put(&solenoid, 0, 1);
            if(autobrew_plan.state.pump_setting_changed){
                ulka_pump_pwr_percent(&pump, autobrew_plan.state.pump_setting);
            }
        } else {
            ulka_pump_off(&pump);
            binary_output_put(&solenoid, 0, 0);
        }
    }

    // Update Pump States
    _state.pump.pump_lock     = ulka_pump_get_lock(&pump);
    _state.pump.power_level   = ulka_pump_get_pwr(&pump);
    _state.pump.flowrate_ml_s = flow_meter_rate(&flow);
    _state.pump.pressure_bar  = ulka_pump_get_pressure(&pump);
}

/**
 * \brief Updates the boiler's setpoint based on the machine's mode, saves its state,
 * and ticks its controller.
 */
static void espresso_machine_update_boiler(){
    // Update setpoints
    if(_state.switches.ac_switch){
        if(_state.switches.mode_dial == MODE_STEAM){
            _state.boiler.setpoint = 1.6*(*settings->steam.temp);
        } else if(_state.switches.mode_dial == MODE_HOT){
            _state.boiler.setpoint = 1.6*(*settings->hot.temp);
        } else {
            _state.boiler.setpoint = 1.6*(*settings->brew.temp);
        }
    } else {
        _state.boiler.setpoint = 0;
    }

    heater_pid.setpoint = _state.boiler.setpoint/16.;
    _state.boiler.temperature = lmt01_read(&thermo);

    pid_tick(&heater_pid);
}

/**
 * \brief Updates the 3 LEDs on the front of the machine.
 * 
 * If the machine is off, then the settings UI mask is applied to the LEDs. Otherwise, the LED 0
 * is on (indicating the machine is powered), LED 1 is on if the boiler is at it's setpoint, and LED 2
 * is on if the pump is off and the scale is at the required dose.
 */
static void espresso_machine_update_leds(){
    if(_state.switches.ac_switch){
        const bool led0 = _state.switches.ac_switch;
        const bool led1 = _state.switches.ac_switch && pid_at_setpoint(&heater_pid, 2.5);
        const bool led2 = _state.switches.ac_switch 
                          && !_state.switches.pump_switch 
                          && nau7802_at_val_mg(&scale, *settings->brew.dose *100);
        binary_output_put(&leds, 0, led0);
        binary_output_put(&leds, 1, led1);
        binary_output_put(&leds, 2, led2);
    } else {
        binary_output_mask(&leds, settings->ui_mask);
    }
}

int espresso_machine_setup(espresso_machine_viewer * state_viewer){
    if(state_viewer != NULL){
        *state_viewer = &_state;
    }

    i2c_bus_setup(bus, 100000, I2C_SCL_PIN, I2C_SDA_PIN);

    mb85_fram_setup(&mem, bus, 0x00, NULL);

    settings = machine_settings_setup(&mem);

    // Setup the autobrew first leg that does not depend on machine settings
    autobrew_routine_setup(&autobrew_plan, 6);
    autobrew_setup_function_call_leg(&autobrew_plan, 0, 0, &zero_scale);

    // Setup heater as a slow_pwm object
    slow_pwm_setup(&heater, HEATER_PWM_PIN, 1260, 64);
    heater_pid.K.p = PID_GAIN_P;
    heater_pid.K.i = PID_GAIN_I;
    heater_pid.K.d = PID_GAIN_D;
    heater_pid.K.f = PID_GAIN_F;
    heater_pid.min_time_between_ticks_ms = 100;
    heater_pid.sensor = &read_boiler_thermo;
    heater_pid.sensor_feedforward = &read_pump_flowrate;
    heater_pid.plant = &apply_boiler_input;
    heater_pid.setpoint = 0;
    pid_init(&heater_pid, 0, 175, 1000);

    // Setup the pressure sensor
    /** \todo Utilize analog input */
    analog_input_setup(&pressure_sensor, PRESSURE_SENSOR_PIN, PRESSURE_CONVERSION_BAR);

    // Setup the LED binary output
    const uint8_t led_pins[3] = {LED0_PIN, LED1_PIN, LED2_PIN};
    binary_output_setup(&leds, led_pins, 3);

    // Setup the binary inputs for pump switch and mode dial.
    const uint8_t pump_switch_gpio = PUMP_SWITCH_PIN;
    const uint8_t mode_select_gpio[2] = {DIAL_A_PIN, DIAL_B_PIN};

    binary_input_setup(&pump_switch, 1, &pump_switch_gpio, BINARY_INPUT_PULL_UP, 10000, false, false);
    binary_input_setup(&mode_dial, 2, mode_select_gpio, BINARY_INPUT_PULL_UP, 75000, false, true);

    // Setup the pump
    ulka_pump_setup(&pump,AC_0CROSS_PIN, PUMP_OUT_PIN, AC_0CROSS_SHIFT, ZEROCROSS_EVENT_RISING);

    // Setup solenoid as a binary output
    uint8_t solenoid_pin [1] = {SOLENOID_PIN};
    binary_output_setup(&solenoid, solenoid_pin, 1);

    // Setup nau7802 and flowrate tracker
    nau7802_setup(&scale, bus, SCALE_CONVERSION_MG);
    discrete_derivative_init(&scale_flowrate, 250);

    // Setup thermometer
    lmt01_setup(&thermo, 0, LMT01_DATA_PIN);

    // Setup flow meter
    flow_meter_setup(&flow, FLOW_RATE_PIN, FLOW_CONVERSION_ML);

    // Setup AC power sensor
    gpio_irq_timestamp_setup(AC_0CROSS_PIN, ZEROCROSS_EVENT_RISING);

    espresso_machine_update_state();

    machine_settings_print();
    return 0;
}

void espresso_machine_tick(){
    espresso_machine_update_switches();
    espresso_machine_update_settings();
    espresso_machine_update_boiler();
    espresso_machine_update_pump();
    espresso_machine_update_leds();
}