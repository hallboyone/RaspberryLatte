/**
 * \file espresso_machine.c
 * \ingroup espresso_machine
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Espresso Machine source
 * \version 0.1
 * \date 2022-12-09
 */

//#define DISABLE_BOILER

#include "machine_logic/espresso_machine.h"

#include <stdio.h>

#include "config/raspberry_latte_config.h"
#include "pinout.h"

#include "machine_logic/autobrew.h"
#include "machine_logic/machine_settings.h"

#include "drivers/nau7802.h"
#include "drivers/lmt01.h"
#include "drivers/ulka_pump.h"

#include "utils/thermal_runaway_watcher.h"
#include "utils/gpio_irq_timestamp.h"
#include "utils/binary_output.h"
#include "utils/binary_input.h"
#include "utils/slow_pwm.h"
#include "utils/i2c_bus.h"
#include "utils/pid.h"
#include "utils/macros.h"

/** An internal variable that collects the current state of the machine. */
static espresso_machine_state _state = {.pump.pump_lock = true, .boiler.temperature = 0}; 

/** I2C bus connected to memory, scale ADC, and I2C port */
static i2c_inst_t *  bus = i2c1;

static thermal_runaway_watcher trw;         /**< Watches the boiler state to make sure it's behaving as expected. */
static binary_output           leds;        /**< The three LEDs indicating to the user the machine's state. */
static binary_input            pump_switch; /**< Monitors the state of the pump switch. */
static binary_input            mode_dial;   /**< Monitors the state of the mode dial. */
static absolute_time_t         ac_on_time;  /**< Records the time of when the machine was switched on. */
static binary_output           solenoid;    /**< Drives the digital output for the solenoid. */
static slow_pwm                heater;      /**< PWM signal for the SSR driving the boiler. */
static lmt01                   thermo;      /**< Boiler thermometer. */
static nau7802                 scale;       /**< Output scale. */
static ulka_pump               pump;        /**< The vibratory pump. */

static const machine_settings* settings;

/** Autobrew and control objects */
static pid  heater_pid;
static pid  flow_pid;                    

/** 
 * \brief Shuts down the pump and boiler. 
*/
static void espresso_machine_e_stop();

/**
 * \brief Checks if the last AC zerocross time was within 1 60hz period.
 * \return True AC is on. False otherwise.
 */
static inline bool is_ac_on(){
    const int64_t period_60hz_us = 1000000/60;
    return (gpio_irq_timestamp_read_duration_us(AC_0CROSS_PIN) < period_60hz_us);
}

/** 
 * \brief Checks if AC has been on for the AC_SETTLING_TIME_MS so that transient spikes can die out. 
 */
static inline bool is_ac_settled(){
    return absolute_time_diff_us(ac_on_time, get_absolute_time()) > 1000*AC_SETTLING_TIME_MS;
}

/** 
 * \brief Returns the boiler temp in C. 
 * Helper function for the boiler PID controller. 
 */
static pid_data read_boiler_thermo_C(){
    return lmt01_read_float(thermo);
}

/**
 * \brief Applies an input to the boiler heater.
 * \param u Output for the boiler. 1 is full on, 0 is full off.
 */
static void apply_boiler_input(float u){
    slow_pwm_set_float_duty(heater, u);
}

/** 
 * \brief Returns the pumps's flowrate.
 * Used by the flow control's PID controller.
 */
static pid_data read_pump_flowrate_ul_s(){
    return 1000.0*ulka_pump_get_flow_ml_s(pump);
}

/** 
 * \brief Zeros the scale. 
 * Helper for autobrew routine.
 */
static void zero_scale(){
    nau7802_zero(scale);
}

/** 
 * \brief Resets flow control and sets the bias to the current pump power.
 * Helper for the autobrew routine.
 */
static void setup_flow_ctrl(){
    pid_reset(flow_pid);
    pid_update_bias(flow_pid, ulka_pump_get_pwr(pump));
}

/** 
 * \brief Checks if the scale is greater than or equal to the passed in value. 
 */
static bool scale_at_val(uint16_t val_mg){
    return nau7802_at_val_mg(scale, val_mg);
}

/** 
 * \brief Checks if flowrate is greater than or equal to the passed in value. 
 */
static bool system_at_flow(uint16_t flow_ul_ds){
    return 100.0*ulka_pump_get_flow_ml_s(pump) >= flow_ul_ds;
}

/** 
 * \brief Checks if pump is under passed in pressure 
 */
static bool system_at_pressure(uint16_t pressure_mbar){
    return ulka_pump_get_pressure_bar(pump) > (pressure_mbar/1000.);
}

/** 
 * \brief Returns the pump power required to hit target pressure (clipped between 0 and 100).
 * \param target_pressure_bar The pressure in bar that is targeted.
 * \returns The pump power needed to hit the target pressure in percent power.
*/
static uint8_t get_power_for_pressure(uint16_t target_pressure_mbar){
    return ulka_pump_pressure_to_power(pump, target_pressure_mbar/1000.0);
}

/** \brief Returns the pump power needed to regulated to the target flow rate.
 * 
 * The flow_ctrl PID object is updated to the new setpoint, ticked, and the current input is returned.
 * 
 * \param target_flow_ul_ds The target flowrate in ul/ds.
 * \returns The pump power needed to reach the target flowrate, according to the flow_ctrl PID object.
*/
static uint8_t get_power_for_flow(uint16_t target_flow_ul_ds){
    pid_update_setpoint(flow_pid, target_flow_ul_ds*10);
    return (uint8_t)pid_tick(flow_pid, NULL);
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
    const uint32_t ramp_t  = *settings->autobrew.preinf_ramp_time;
    const uint32_t pre_t   = *settings->autobrew.preinf_timeout * 10;
    const uint8_t  pre_pwr = *settings->autobrew.preinf_power;
    const uint32_t brew_t  = *settings->autobrew.timeout * 10;
    const uint16_t f_ul_ds = *settings->autobrew.flow;
    const uint16_t yield   = *settings->brew.yield;
    
    uint8_t leg_id;
    autobrew_init();
    leg_id = autobrew_add_leg(NULL, 0, pre_pwr, ramp_t);
    autobrew_leg_add_setup_fun(leg_id, zero_scale);

    leg_id = autobrew_add_leg(NULL, pre_pwr, pre_pwr, pre_t);
    autobrew_leg_add_trigger(leg_id, system_at_pressure, AUTOBREW_PREINF_END_PRESSURE_MBAR);

    leg_id = autobrew_add_leg(get_power_for_pressure, AUTOBREW_PREINF_END_PRESSURE_MBAR, AUTOBREW_BREW_PRESSURE_MBAR, 2*ramp_t);
    
    leg_id = autobrew_add_leg(get_power_for_pressure, AUTOBREW_BREW_PRESSURE_MBAR, AUTOBREW_BREW_PRESSURE_MBAR, brew_t);
    autobrew_leg_add_trigger(leg_id, system_at_flow, f_ul_ds);

    leg_id = autobrew_add_leg(get_power_for_flow, f_ul_ds, f_ul_ds, brew_t);
    autobrew_leg_add_trigger(leg_id, scale_at_val, yield);
    autobrew_leg_add_setup_fun(leg_id, setup_flow_ctrl);
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

        // If machine has been switched on...
        if(new_ac_switch){
            ac_on_time = get_absolute_time();
            espresso_machine_autobrew_setup();
            pid_reset(heater_pid);
        }
    } else {
        _state.switches.ac_switch_changed = 0;
    }

    const bool new_pump_switch = binary_input_read(pump_switch);
    if(_state.switches.pump_switch != new_pump_switch){
        _state.switches.pump_switch_changed = (_state.switches.pump_switch ? -1 : 1);
        _state.switches.pump_switch = new_pump_switch;
    } else {
        _state.switches.pump_switch_changed = 0;
    }

    const int new_mode_switch = binary_input_read(mode_dial);
    if(_state.switches.mode_dial != new_mode_switch){
        _state.switches.mode_dial_changed = (_state.switches.mode_dial > new_mode_switch ? -1 : 1);
        _state.switches.mode_dial = new_mode_switch;
        nau7802_zero(scale);
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
    if(!_state.switches.ac_switch || !is_ac_settled() || thermal_runaway_watcher_error(trw)
       || (_state.switches.pump_switch && (_state.switches.mode_dial_changed || ulka_pump_is_locked(pump)))){
        ulka_pump_lock(pump);
    } else {
        ulka_pump_unlock(pump);
    }

    if(!_state.switches.pump_switch 
        || ulka_pump_is_locked(pump) 
        || MODE_STEAM == _state.switches.mode_dial){
        // If the pump is locked, switched off, or in steam mode
        autobrew_reset();
        ulka_pump_off(pump);
        binary_output_put(solenoid, 0, 0);

    } else if (MODE_HOT == _state.switches.mode_dial){
        ulka_pump_pwr_percent(pump, *settings->hot.power);
        binary_output_put(solenoid, 0, 0);

    } else if (MODE_MANUAL == _state.switches.mode_dial){
        ulka_pump_pwr_percent(pump, *settings->brew.power);
        binary_output_put(solenoid, 0, 1);

    } else if (MODE_AUTO ==_state.switches.mode_dial){
        if(!autobrew_routine_tick()){
            binary_output_put(solenoid, 0, 1);
            if(autobrew_pump_changed()){
                ulka_pump_pwr_percent(pump, autobrew_pump_power());
            }
            _state.autobrew_leg = 1+autobrew_current_leg();
        } else {
            ulka_pump_off(pump);
            binary_output_put(solenoid, 0, 0);
            _state.autobrew_leg = 0;
        }
    }

    // Update Pump States
    _state.pump.pump_lock     = ulka_pump_is_locked(pump);
    _state.pump.power_level   = ulka_pump_get_pwr(pump);
    _state.pump.flowrate_ml_s = read_pump_flowrate_ul_s();
    _state.pump.pressure_bar  = ulka_pump_get_pressure_bar(pump);
}

/**
 * \brief Updates the boiler's setpoint based on the machine's mode, saves its state,
 * and ticks its controller.
 */
static void espresso_machine_update_boiler(){
    _state.boiler.temperature = lmt01_read(thermo);

    #ifdef DISABLE_BOILER
    _state.boiler.setpoint = 0;
    #else
    // Update setpoints
    if(_state.switches.ac_switch && is_ac_settled()){
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

    if(thermal_runaway_watcher_tick(trw, _state.boiler.setpoint, _state.boiler.temperature) < 0){
        espresso_machine_e_stop(); // Shut down pump and boiler
        _state.boiler.setpoint = 0;
    }
    else {
        // If no thermal runaway
        pid_update_setpoint(heater_pid, _state.boiler.setpoint/16.);
        pid_tick(heater_pid, &_state.boiler.pid_state);
    }
    #endif
    
    _state.boiler.power_level = slow_pwm_get_duty(heater);
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
        uint8_t led_state = 0;
        if(thermal_runaway_watcher_error(trw)){
            // If errored out, flash error state
            const uint LED_TOGGLE_PERIOD_MS = 512;
            if (to_ms_since_boot(get_absolute_time())%LED_TOGGLE_PERIOD_MS > (LED_TOGGLE_PERIOD_MS/2)){
                led_state = (1 << (3+thermal_runaway_watcher_state(trw)));
            }
        } else {
            led_state = (_state.switches.ac_switch) << 2|
                        (_state.switches.ac_switch && pid_at_setpoint(heater_pid, 2.5)) << 1 |
                        (_state.switches.ac_switch && !_state.switches.pump_switch 
                        && nau7802_at_val_mg(scale, *settings->brew.dose *100)) << 0;
        }
        binary_output_mask(leds, led_state);
    } else {
        binary_output_mask(leds, settings->ui_mask);
    }
}

int espresso_machine_setup(espresso_machine_viewer * state_viewer){
    if(state_viewer != NULL){
        *state_viewer = &_state;
    }

    i2c_bus_setup(bus, 100000, I2C_SCL_PIN, I2C_SDA_PIN);

    settings = machine_settings_setup(mb85_fram_setup(bus, 0x00, NULL));

    autobrew_init();

    // Setup flow control PID object
    const pid_gains flow_K = {.p = FLOW_PID_GAIN_P, .i = FLOW_PID_GAIN_I, .d = FLOW_PID_GAIN_D, .f = FLOW_PID_GAIN_F};
    flow_pid = pid_setup(flow_K, &read_pump_flowrate_ul_s, NULL, NULL, -100, 100, 25, 100);

    // Setup heater as a slow_pwm object
    heater = slow_pwm_setup(HEATER_PWM_PIN, 1260, 64);
    const pid_gains boiler_K = {.p = BOILER_PID_GAIN_P, .i = BOILER_PID_GAIN_I, .d = BOILER_PID_GAIN_D, .f = BOILER_PID_GAIN_F};
    heater_pid = pid_setup(boiler_K, &read_boiler_thermo_C, &read_pump_flowrate_ul_s, &apply_boiler_input, 0, 1, 100, 1000);
    trw = thermal_runaway_watcher_setup(THERMAL_RUNAWAY_WATCHER_MAX_CONSECUTIVE_TEMP_CHANGE_16C,
                                        THERMAL_RUNAWAY_WATCHER_CONVERGENCE_TOL_16C,
                                        THERMAL_RUNAWAY_WATCHER_DIVERGENCE_TOL_16C,
                                        THERMAL_RUNAWAY_WATCHER_MIN_TEMP_CHANGE_HEAT_16C,
                                        THERMAL_RUNAWAY_WATCHER_MIN_TEMP_CHANGE_COOL_16C,
                                        THERMAL_RUNAWAY_WATCHER_MIN_TEMP_CHANGE_PERIOD_MS);
    
    // Setup the LED binary output
    const uint8_t led_pins[3] = {LED0_PIN, LED1_PIN, LED2_PIN};
    leds = binary_output_setup(led_pins, 3);

    // Setup the binary inputs for pump switch and mode dial.
    const uint8_t pump_switch_gpio = PUMP_SWITCH_PIN;
    const uint8_t mode_select_gpio[2] = {DIAL_A_PIN, DIAL_B_PIN};

    pump_switch = binary_input_setup(1, &pump_switch_gpio, BINARY_INPUT_PULL_UP, PUMP_SWITCH_DEBOUNCE_DURATION_US, false, false);
    mode_dial = binary_input_setup(2, mode_select_gpio, BINARY_INPUT_PULL_UP, MODE_DIAL_DEBOUNCE_DURATION_US, false, true);

    // Setup the pump
    pump = ulka_pump_setup(AC_0CROSS_PIN, PUMP_OUT_PIN, AC_0CROSS_SHIFT, ZEROCROSS_EVENT_RISING);
    ulka_pump_setup_flow_meter(pump, FLOW_RATE_PIN, PULSE_TO_FLOW_CONVERSION_ML);

    // Setup solenoid as a binary output
    uint8_t solenoid_pin [1] = {SOLENOID_PIN};
    solenoid = binary_output_setup(solenoid_pin, 1);

    // Setup nau7802
    scale = nau7802_setup(bus, SCALE_CONVERSION_MG);

    // Setup thermometer
    thermo = lmt01_setup(0, LMT01_DATA_PIN);

    // Setup AC power sensor
    gpio_irq_timestamp_setup(AC_0CROSS_PIN, ZEROCROSS_EVENT_RISING);

    espresso_machine_update_settings();

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

static void espresso_machine_e_stop(){
    slow_pwm_set_duty(heater, 0);
    ulka_pump_off(pump);
    binary_output_mask(solenoid, 0);
}