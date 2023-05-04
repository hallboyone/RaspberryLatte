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

const float BOILER_PID_GAIN_P = 0.05;
const float BOILER_PID_GAIN_I = 0.00000175;
const float BOILER_PID_GAIN_D = 0.0005;
const float BOILER_PID_GAIN_F = 0.00005;
const float FLOW_PID_GAIN_P = 0.0125;
const float FLOW_PID_GAIN_I = 0.00004;
const float FLOW_PID_GAIN_D = 0.0;
const float FLOW_PID_GAIN_F = 0.0;

const float    SCALE_CONVERSION_MG = -0.152710615479;
const float FLOW_CONVERSION_UL = 0.5; /**< ml per pulse of pump flow sensor. */

static espresso_machine_state _state = {.pump.pump_lock = true, .boiler.temperature = 0}; 

/** I2C bus connected to RTC, memory, scale ADC, and I2C port */
static i2c_inst_t *  bus = i2c1;

/** All the peripheral components for the espresso machine */
static thermal_runaway_watcher trw;
static binary_output       leds;
static binary_input        pump_switch, mode_dial;
static absolute_time_t     ac_switched_on_time;
static binary_output       solenoid;
static slow_pwm            heater;
static lmt01               thermo; 
static nau7802             scale;
static ulka_pump           pump;

static const machine_settings*   settings;

/** Autobrew and control objects */
static pid         heater_pid;
static pid         flow_pid;
typedef enum {RESET_SCALE = 0,
              PREINF_RAMP,
              PREINF_ON,
              PREPARE_FLOW,
              FLOW_CTRL,
              NUM_LEGS} AUTOBREW_LEGS;

static autobrew_routine autobrew_plan;

static void espresso_machine_e_stop();

/**
 * \brief Helper function for the PID controller. Returns the boiler temp in C.
 */
static pid_data read_boiler_thermo_C(){
    return lmt01_read_float(thermo);
}

/**
 * \brief Helper function that tracks and returns the scale's flowrate
*/
static pid_data read_pump_flowrate_ul_s(){
    return 1000.0*ulka_pump_get_flow_ml_s(pump);
}

/**
 * \brief Helper function for the PID controller. Applies an input to the boiler heater.
 * 
 * \param u Output for the boiler. 1 is full on, 0 is full off.
 */
static void apply_boiler_input(float u){
    slow_pwm_set_float_duty(heater, u);
}

/** 
 * \brief Returns true if scale is greater than or equal to the current output. 
 */
static bool scale_at_output(){
    return nau7802_at_val_mg(scale, *settings->brew.yield*100);
}

/**
 * \brief Helper function to zero the scale. Used by the autobrew routine. 
 */
static void zero_scale(){
    nau7802_zero(scale);
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

/** \brief Checks if AC has been on for 200ms so that transient spikes can die out. */
static inline bool is_ac_settled(){
    return absolute_time_diff_us(ac_switched_on_time, get_absolute_time()) > 1000*AC_SETTLING_TIME_MS;
}

/** \brief Resets the flow-control PID and sets its bias */
static inline void flow_pid_reset(){
    pid_reset(flow_pid);
}

/** \brief Returns true if system is under moderate (3 bars) pressure */
static inline bool system_under_pressure(){
    return ulka_pump_get_pressure_bar(pump) > 3.0;
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
    const uint32_t preinf_ramp_dur = *settings->autobrew.preinf_ramp_time * 100000UL;
    const uint32_t preinf_on_dur   = *settings->autobrew.preinf_timeout * 1000000UL;
    const uint8_t  preinf_pwr      = *settings->autobrew.preinf_power;
    const uint32_t brew_on_dur     = *settings->autobrew.timeout * 1000000UL;
    const uint32_t brew_flow       = *settings->autobrew.flow * 10UL;

    autobrew_setup_linear_setpoint_leg(&autobrew_plan, PREINF_RAMP, 0,          preinf_pwr, NULL,     preinf_ramp_dur, NULL);
    autobrew_setup_linear_setpoint_leg(&autobrew_plan, PREINF_ON,   preinf_pwr, preinf_pwr, NULL,     preinf_on_dur,   &system_under_pressure);
    autobrew_setup_linear_setpoint_leg(&autobrew_plan, FLOW_CTRL,   brew_flow,  brew_flow,  flow_pid, brew_on_dur,     &scale_at_output);

    pid_update_bias(flow_pid, preinf_pwr);
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
            ac_switched_on_time = get_absolute_time();
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
        autobrew_routine_reset(&autobrew_plan);
        ulka_pump_off(pump);
        binary_output_put(solenoid, 0, 0);

    } else if (MODE_HOT == _state.switches.mode_dial){
        ulka_pump_pwr_percent(pump, *settings->hot.power);
        binary_output_put(solenoid, 0, 0);

    } else if (MODE_MANUAL == _state.switches.mode_dial){
        ulka_pump_pwr_percent(pump, *settings->brew.power);
        binary_output_put(solenoid, 0, 1);

    } else if (MODE_AUTO ==_state.switches.mode_dial){
        if(!autobrew_routine_tick(&autobrew_plan)){
            binary_output_put(solenoid, 0, 1);
            if(autobrew_plan.state.pump_setting_changed){
                ulka_pump_pwr_percent(pump, autobrew_plan.state.pump_setting);
            }
        } else {
            ulka_pump_off(pump);
            binary_output_put(solenoid, 0, 0);
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

    _state.boiler.temperature = lmt01_read(thermo);

    if(thermal_runaway_watcher_tick(trw, _state.boiler.setpoint, _state.boiler.temperature) < 0){
        espresso_machine_e_stop(); // Shut down pump and boiler
        _state.boiler.setpoint = 0;
    }
    #ifdef DISABLE_BOILER
    _state.boiler.setpoint = 0;
    #else
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

    // Setup the autobrew first leg that does not depend on machine settings
    autobrew_routine_setup(&autobrew_plan, NUM_LEGS);
    autobrew_setup_function_call_leg(&autobrew_plan, RESET_SCALE, 0, &zero_scale);
    autobrew_setup_function_call_leg(&autobrew_plan, PREPARE_FLOW, 15, &flow_pid_reset);
    const pid_gains flow_K = {.p = FLOW_PID_GAIN_P, .i = FLOW_PID_GAIN_I, .d = FLOW_PID_GAIN_D, .f = FLOW_PID_GAIN_F};
    flow_pid = pid_setup(flow_K, &read_pump_flowrate_ul_s, NULL, NULL, 0, 100, 25, 100);

    // Setup heater as a slow_pwm object
    heater = slow_pwm_setup(HEATER_PWM_PIN, 1260, 64);
    const pid_gains boiler_K = {.p = BOILER_PID_GAIN_P, .i = BOILER_PID_GAIN_I, .d = BOILER_PID_GAIN_D, .f = BOILER_PID_GAIN_F};
    heater_pid = pid_setup(boiler_K, &read_boiler_thermo_C, &read_pump_flowrate_ul_s, 
              &apply_boiler_input, 0, 1, 100, 1000);
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

    pump_switch = binary_input_setup(1, &pump_switch_gpio, BINARY_INPUT_PULL_UP, 10000, false, false);
    mode_dial = binary_input_setup(2, mode_select_gpio, BINARY_INPUT_PULL_UP, 75000, false, true);

    // Setup the pump
    pump = ulka_pump_setup(AC_0CROSS_PIN, PUMP_OUT_PIN, AC_0CROSS_SHIFT, ZEROCROSS_EVENT_RISING);
    ulka_pump_setup_flow_meter(pump, FLOW_RATE_PIN, FLOW_CONVERSION_UL);

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