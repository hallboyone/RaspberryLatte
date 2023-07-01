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

/** Autobrew and control objects */
static pid  heater_pid;
static pid  flow_pid;                    

/** 
 * \brief Shuts down the pump and boiler. 
*/
static void espresso_machine_e_stop();

/**
 * \brief Checks if the AC is on.
 * If the last AC zerocross time was within 17,000us (a little over a 60hz 
 * period), then it's assumed on.
 * \return True AC is on. False otherwise.
 */
static inline bool is_ac_on(){
    return (gpio_irq_timestamp_read_duration_us(AC_0CROSS_PIN) < 17000);
}

/** 
 * \brief Checks if AC has settled.
 * The AC is assumed settled once AC_SETTLING_TIME_MS have passed and AC is on. 
 * The time should be long enough to allow transient spikes to die out. 
 * \return True if ac has settled. Else, returns false.
 */
static inline bool is_ac_on_and_settled(){
    return is_ac_on() && absolute_time_diff_us(ac_on_time, get_absolute_time()) > 1000*AC_SETTLING_TIME_MS;
}

/** 
 * \brief Getter for the boiler temp. 
 * Used as a helper function for the boiler PID controller.
 * \returns The current boiler temp in C. 
 */
static pid_data read_boiler_thermo_C(){
    return lmt01_read_float(thermo);
}

/** 
 * \brief Getter for the pump's flowrate. 
 * Used as a helper function for the pumps flow controller.
 * \returns The current flow rate through the pump in ul/s. 
 */
static pid_data read_pump_flowrate_ul_s(){
    return 1000.0*ulka_pump_get_flow_ml_s(pump);
}

/** 
 * \brief Setter for the boiler's duty cycle. 
 * Used as a helper function for the boiler PID controller.
 * \param u A duty cycle with 0 being off and 1 being full on.
 */
static void apply_boiler_input(float u){
    slow_pwm_set_float_duty(heater, u);
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
static bool system_at_flow(uint16_t flow_ul_s){
    return 1000.0*ulka_pump_get_flow_ml_s(pump) >= flow_ul_s;
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
 * \param target_flow_ul_s The target flowrate in ul/s.
 * \returns The pump power needed to reach the target flowrate, according to the flow_ctrl PID object.
*/
static uint8_t get_power_for_flow(uint16_t target_flow_ul_s){
    pid_update_setpoint(flow_pid, target_flow_ul_s);
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
    autobrew_init();
    uint8_t leg_id;
    bool is_first_leg = false;
    for(uint8_t i = 0; i < NUM_AUTOBREW_LEGS; i++){
        const uint8_t offset = i*NUM_AUTOBREW_PARAMS_PER_LEG;
        const machine_setting leg_timeout = machine_settings_get(MS_A1_TIMEOUT_s + offset);
        if(leg_timeout > 0){
            // Setup reference and timeout
            const machine_setting ref_style = machine_settings_get(MS_A1_REF_STYLE_ENM + offset);
            const machine_setting ref_start = machine_settings_get(MS_A1_REF_START_100per_ulps_mbar + offset);
            const machine_setting ref_end   = machine_settings_get(MS_A1_REF_END_100per_ulps_mbar + offset);
            if(ref_style == AUTOBREW_REF_STYLE_PWR){
                leg_id = autobrew_add_leg(NULL, ref_start/100, ref_end/100, leg_timeout);
            } else if(ref_style == AUTOBREW_REF_STYLE_FLOW){
                leg_id = autobrew_add_leg(get_power_for_flow, ref_start, ref_end, leg_timeout);
                autobrew_leg_add_setup_fun(leg_id, setup_flow_ctrl);
            } else {
                leg_id = autobrew_add_leg(get_power_for_pressure, ref_start, ref_end, leg_timeout); 
            }

            // Setup triggers
            const machine_setting t_flow = machine_settings_get(MS_A1_TRGR_FLOW_ul_s + offset);
            if(t_flow>0) autobrew_leg_add_trigger(leg_id, system_at_flow, t_flow);
            
            const machine_setting t_prsr = machine_settings_get(MS_A1_TRGR_PRSR_mbar + offset);
            if(t_prsr>0) autobrew_leg_add_trigger(leg_id, system_at_pressure, t_prsr);
            
            const machine_setting t_mass = machine_settings_get(MS_A1_TRGR_MASS_mg + offset);
            if(t_mass>0) autobrew_leg_add_trigger(leg_id, scale_at_val, t_mass);

            // Zero scale on first non-zero leg found
            if(is_first_leg){
                is_first_leg = false;
                autobrew_leg_add_setup_fun(leg_id, zero_scale);
            }
        }
    }
}

/**
 * \brief Flag changes in switch values and run simple event-driven routines.
 * 
 * The AC switch sets flags for its current value and if it changed. If switched on,
 * record the current time, setup the autobrew routine, and reset the heater PID.
 * 
 * The mode dial sets flags for its current value and if it changed. If it does 
 * change, the scale is zeroed for weighing beans.
 * 
 * The pump switch sets flags for its current value and if it changed.
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
        nau7802_zero(scale); // zero scale so we can weigh the beans
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
    if(!is_ac_on_and_settled() || thermal_runaway_watcher_errored(trw)
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
        ulka_pump_pwr_percent(pump, machine_settings_get(MS_POWER_HOT_PER));
        binary_output_put(solenoid, 0, 0);
    } else if (MODE_MANUAL == _state.switches.mode_dial){
        ulka_pump_pwr_percent(pump, machine_settings_get(MS_POWER_BREW_PER));
        binary_output_put(solenoid, 0, 1);
    } else if (MODE_AUTO == _state.switches.mode_dial){
        if(!autobrew_routine_tick()){
            binary_output_put(solenoid, 0, 1);
            if(autobrew_pump_changed()){
                ulka_pump_pwr_percent(pump, autobrew_pump_power());
            }
            _state.autobrew_leg = 1 + autobrew_current_leg(); // legs are 0 indexed, shifted here to 1
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
    if(is_ac_on_and_settled()){
        if(_state.switches.mode_dial == MODE_STEAM){
            _state.boiler.setpoint = machine_settings_get(MS_TEMP_STEAM_cC);
        } else if(_state.switches.mode_dial == MODE_HOT){
            _state.boiler.setpoint = machine_settings_get(MS_TEMP_HOT_cC);
        } else {
            _state.boiler.setpoint = machine_settings_get(MS_TEMP_BREW_cC);
        }
    } else {
        _state.boiler.setpoint = 0;
    }

    _state.boiler.thermal_state = thermal_runaway_watcher_tick(trw, _state.boiler.setpoint, _state.boiler.temperature, !_state.switches.ac_switch);
    if(thermal_runaway_watcher_errored(trw)){
        espresso_machine_e_stop(); // Shut down pump and boiler
        _state.boiler.setpoint = 0;
    } else {
        pid_update_setpoint(heater_pid, _state.boiler.setpoint/100.);
        pid_tick(heater_pid, &_state.boiler.pid_state);
    }
    _state.boiler.power_level = slow_pwm_get_duty(heater);
    #endif
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
        if(thermal_runaway_watcher_errored(trw)){
            // If errored out, flash error state
            if (to_ms_since_boot(get_absolute_time())%THERMAL_RUNAWAY_WATCHER_LED_TOGGLE_PERIOD_MS 
                    > (THERMAL_RUNAWAY_WATCHER_LED_TOGGLE_PERIOD_MS/2)){
                led_state = (1 << (3+thermal_runaway_watcher_state(trw)));
            }
        } else {
            led_state = (_state.switches.ac_switch) << 2|
                        (_state.switches.ac_switch && pid_at_setpoint(heater_pid, 2.5)) << 1 |
                        (_state.switches.ac_switch && !_state.switches.pump_switch 
                        && nau7802_at_val_mg(scale, machine_settings_get(MS_WEIGHT_DOSE_mg))) << 0;
        }
        binary_output_mask(leds, led_state);
    } else {
        binary_output_mask(leds, machine_settings_get(MS_UI_MASK));
    }
}

int espresso_machine_setup(espresso_machine_viewer * state_viewer){
    if(state_viewer != NULL){
        *state_viewer = &_state;
    }

    i2c_bus_setup(bus, 100000, I2C_SCL_PIN, I2C_SDA_PIN);

    machine_settings_setup(mb85_fram_setup(bus, 0x00, NULL));

    autobrew_init();

    // Setup flow control PID object
    const pid_gains flow_K = {.p = FLOW_PID_GAIN_P, .i = FLOW_PID_GAIN_I, .d = FLOW_PID_GAIN_D, .f = FLOW_PID_GAIN_F};
    flow_pid = pid_setup(flow_K, &read_pump_flowrate_ul_s, NULL, NULL, -100, 100, 25, 100);

    // Setup heater as a slow_pwm object
    heater = slow_pwm_setup(HEATER_PWM_PIN, 1260, 64);
    const pid_gains boiler_K = {.p = BOILER_PID_GAIN_P, .i = BOILER_PID_GAIN_I, .d = BOILER_PID_GAIN_D, .f = BOILER_PID_GAIN_F};
    heater_pid = pid_setup(boiler_K, &read_boiler_thermo_C, &read_pump_flowrate_ul_s, &apply_boiler_input, 0, 1, 100, 1000);
    trw = thermal_runaway_watcher_setup(THERMAL_RUNAWAY_WATCHER_MAX_CONSECUTIVE_TEMP_CHANGE_cC,
                                        THERMAL_RUNAWAY_WATCHER_CONVERGENCE_TOL_cC,
                                        THERMAL_RUNAWAY_WATCHER_DIVERGENCE_TOL_cC,
                                        THERMAL_RUNAWAY_WATCHER_MIN_TEMP_CHANGE_HEAT_cC,
                                        THERMAL_RUNAWAY_WATCHER_MIN_TEMP_CHANGE_COOL_cC,
                                        THERMAL_RUNAWAY_WATCHER_MIN_TEMP_CHANGE_PERIOD_MS);
    
    // Setup the LED binary output
    const uint8_t led_pins[3] = {LED0_PIN, LED1_PIN, LED2_PIN};
    leds = binary_output_setup(led_pins, 3);

    // Setup the binary inputs for pump switch and mode dial.
    const uint8_t pump_switch_gpio = PUMP_SWITCH_PIN;
    pump_switch = binary_input_setup(1, &pump_switch_gpio, BINARY_INPUT_PULL_UP, PUMP_SWITCH_DEBOUNCE_DURATION_US, false, false);

    const uint8_t mode_select_gpio[2] = {DIAL_A_PIN, DIAL_B_PIN};
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
    thermo = lmt01_setup(0, LMT01_DATA_PIN, BOILER_TEMP_OFFSET_16C);

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