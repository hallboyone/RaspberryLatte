#include "pinout.h"

#include "i2c_bus.h"

#include "analog_input.h"
#include "binary_output.h"
#include "binary_input.h"
#include "phasecontrol.h"
#include "nau7802.h"
#include "slow_pwm.h"
#include "lmt01.h"
#include "mb85_fram.h"

#include "pid.h"
#include "autobrew.h"

#include "espresso_machine.h"
#include "machine_settings.h"
#include "local_ui.h"
#include "value_flasher.h"
#include "brew_parameters.h"

static espresso_machine_state _state = {.pump.pump_lock = true}; 

/** I2C bus connected to RTC, memory, scale ADC, and I2C port */
static i2c_inst_t *  bus = i2c1;

/** All the peripheral components for the espresso machine */
static analog_input        pressure_sensor;
static binary_output       leds;
static binary_input        pump_switch, mode_dial;
static phasecontrol        pump;
static binary_output       solenoid;
static slow_pwm            heater;
static lmt01               thermo; 
static nau7802             scale;
static discrete_derivative scale_flowrate;
static mb85_fram           mem;

static machine_settings    settings;
static value_flasher       setting_display;

/** Autobrew and control objects */
static pid_ctrl         heater_pid;
static autobrew_leg     autobrew_legs [6];
static autobrew_routine autobrew_plan;

/** Local UI folder objects for updating machine settings */
static local_ui_folder_tree settings_modifier;
static local_ui_folder folder_root;
static local_ui_folder folder_settings;
static local_ui_folder folder_settings_temp;
static local_ui_folder folder_settings_temp_brew;
static local_ui_folder folder_settings_temp_hot;
static local_ui_folder folder_settings_temp_steam;
static local_ui_folder folder_settings_weight;
static local_ui_folder folder_settings_weight_yield;
static local_ui_folder folder_settings_weight_dose;
static local_ui_folder folder_settings_more;
static local_ui_folder folder_settings_more_power;
static local_ui_folder folder_settings_more_power_brew;
static local_ui_folder folder_settings_more_power_hot;
static local_ui_folder folder_settings_more_preinfuse;
static local_ui_folder folder_settings_more_preinfuse_on_time;
static local_ui_folder folder_settings_more_preinfuse_on_power;
static local_ui_folder folder_settings_more_preinfuse_off_time;
static local_ui_folder folder_settings_more_misc;
static local_ui_folder folder_settings_more_misc_timeout;
static local_ui_folder folder_settings_more_misc_ramp_time;
static local_ui_folder folder_presets;
static local_ui_folder folder_presets_profile_a;
static local_ui_folder folder_presets_profile_a_0;
static local_ui_folder folder_presets_profile_a_1;
static local_ui_folder folder_presets_profile_a_2;
static local_ui_folder folder_presets_profile_b;
static local_ui_folder folder_presets_profile_b_0;
static local_ui_folder folder_presets_profile_b_1;
static local_ui_folder folder_presets_profile_b_2;
static local_ui_folder folder_presets_profile_c;
static local_ui_folder folder_presets_profile_c_0;
static local_ui_folder folder_presets_profile_c_1;
static local_ui_folder folder_presets_profile_c_2;

static bool update_machine_setting(folder_id id, uint8_t val){
    if (val > 2) return true;
    const machine_setting deltas [] = {-10, 1, 10};
    if(id == folder_settings_temp_brew.id) {
        machine_settings_set(MS_TEMP_BREW_DC, settings[MS_TEMP_BREW_DC] + deltas[val]);
    } else if(id == folder_settings_temp_hot.id){
        machine_settings_set(MS_TEMP_HOT_DC, settings[MS_TEMP_HOT_DC] + deltas[val]);
    } else if(id == folder_settings_temp_steam.id){
        machine_settings_set(MS_TEMP_STEAM_DC, settings[MS_TEMP_STEAM_DC] + deltas[val]);
    } else if(id == folder_settings_weight_dose.id) {
        machine_settings_set(MS_WEIGHT_DOSE_DG, settings[MS_WEIGHT_DOSE_DG] + deltas[val]);
    } else {
        machine_settings_set(MS_WEIGHT_YIELD_DG, settings[MS_WEIGHT_YIELD_DG] + deltas[val]);
    }
    machine_settings_print();
    return false;
}

static bool save_load_machine_presets(folder_id id, uint8_t val){
    if (val > 2) return true;
    uint8_t profile_id = 0;
    if(id == folder_presets_profile_a_0.id) profile_id = 0;
    else if(id == folder_presets_profile_a_1.id) profile_id = 1;
    else if(id == folder_presets_profile_a_2.id) profile_id = 2;
    else if(id == folder_presets_profile_b_0.id) profile_id = 3;
    else if(id == folder_presets_profile_b_1.id) profile_id = 4;
    else if(id == folder_presets_profile_b_2.id) profile_id = 5;
    else if(id == folder_presets_profile_c_0.id) profile_id = 6;
    else if(id == folder_presets_profile_c_1.id) profile_id = 7;
    else if(id == folder_presets_profile_c_2.id) profile_id = 8;
    if(val == 0){
        machine_settings_save_profile(profile_id);
    } else if(val == 1){
        machine_settings_load_profile(profile_id);
    }
}

/**
 * \brief Helper function for the PID controller. Returns the boiler temp in C.
 */
static float read_boiler_thermo(){
    return lmt01_read_float(&thermo);
}

/**
 * \brief Helper function that tracks and returns the scale's flowrate
*/
static float read_scale_flowrate(){
    return _state.scale.flowrate_mg_s;
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
    return nau7802_at_val_mg(&scale, settings[MS_WEIGHT_YIELD_DG]*100);
}

static int zero_scale(){
    return nau7802_zero(&scale);
}

/**
 * \brief Uses the switch state to update the machine's configuration.
 */
static void espresso_machine_update_state(){
    // Switches
    _state.switches.ac_switch = phasecontrol_is_ac_hot(&pump);
    _state.update_settings = (!_state.switches.ac_switch && _state.switches.pump_switch != binary_input_read(&pump_switch));
    _state.switches.pump_switch = binary_input_read(&pump_switch);

    // Dial
    uint8_t new_mode = binary_input_read(&mode_dial);
    bool mode_changed = (new_mode != _state.switches.mode_dial);
    _state.switches.mode_dial = new_mode;

    //Pump lock
    _state.pump.pump_lock = !_state.switches.ac_switch || (_state.switches.pump_switch && (mode_changed || _state.pump.pump_lock));

    // Update scale
    if(mode_changed){
        nau7802_zero(&scale);
        discrete_derivative_reset(&scale_flowrate);
    }
    _state.scale.val_mg = nau7802_read_mg(&scale);
    datapoint scale_val = {.t = sec_since_boot(), .v = nau7802_read_mg(&scale)};
    _state.scale.flowrate_mg_s = discrete_derivative_add_point(&scale_flowrate, scale_val);

    // Update setpoints
    if(_state.switches.ac_switch){
        if(new_mode == MODE_STEAM){
            _state.boiler.setpoint = 1.6*settings[MS_TEMP_STEAM_DC];
        } else if(new_mode == MODE_HOT){
            _state.boiler.setpoint = 1.6*settings[MS_TEMP_HOT_DC];
        } else {
            _state.boiler.setpoint = 1.6*settings[MS_TEMP_BREW_DC];
        }
    } else {
        _state.boiler.setpoint = 0;
    }
}

static void espresso_machine_update_settings(){
    if (_state.switches.ac_switch){
        // AC on
        _state.settings_viewer_mask = 0;
        local_ui_go_to_root(&settings_modifier);
    } else {
        // AC off
        if(_state.update_settings){
            if(_state.switches.mode_dial == 3){
                local_ui_go_to_root(&settings_modifier);
                _state.settings_viewer_mask = 0;
            } else {
                local_ui_enter_subfolder(&settings_modifier, 2 - _state.switches.mode_dial);
                
                if(local_ui_is_action_folder(settings_modifier.cur_folder)){
                    // If entered action folder, start value flasher
                    const uint32_t id = settings_modifier.cur_folder->id;
                    if(id == folder_settings_temp_brew.id) {
                        value_flasher_setup(&setting_display, settings[MS_TEMP_BREW_DC], 800);
                    } else if(id == folder_settings_temp_hot.id){
                        value_flasher_setup(&setting_display, settings[MS_TEMP_HOT_DC], 800);
                    } else if(id == folder_settings_temp_steam.id){
                        value_flasher_setup(&setting_display, settings[MS_TEMP_STEAM_DC], 800);
                    } else if(id == folder_settings_weight_dose.id) {
                        value_flasher_setup(&setting_display, settings[MS_WEIGHT_DOSE_DG], 800);
                    } else {
                        value_flasher_setup(&setting_display, settings[MS_WEIGHT_YIELD_DG], 800);
                    }
                } else {
                    // else in nav folder. Display id.
                    _state.settings_viewer_mask = 3 - _state.switches.mode_dial;
                }
            }
        }
        // If in action folder, update the value flasher
        if(local_ui_is_action_folder(settings_modifier.cur_folder)){
            _state.settings_viewer_mask = setting_display.out_flags;
        }
    }
}

static void espresso_machine_update_pump(){
    if(!_state.switches.pump_switch){
        heater_pid.K.f = 0;
        autobrew_routine_reset(&autobrew_plan);
        phasecontrol_set_duty_cycle(&pump, 0);
        binary_output_put(&solenoid, 0, 0);
    } else if (_state.pump.pump_lock){
        heater_pid.K.f = 0;
        phasecontrol_set_duty_cycle(&pump, 0);
        binary_output_put(&solenoid, 0, 0);
    } else {
        switch(_state.switches.mode_dial){
            case MODE_STEAM:
                heater_pid.K.f = 0;
                phasecontrol_set_duty_cycle(&pump, 0);
                binary_output_put(&solenoid, 0, 0);
                break;
            case MODE_HOT:
                heater_pid.K.f = 0;
                phasecontrol_set_duty_cycle(&pump, 127);
                binary_output_put(&solenoid, 0, 0);
                break;
            case MODE_MANUAL:
                heater_pid.K.f = 0;
                phasecontrol_set_duty_cycle(&pump, 127);
                binary_output_put(&solenoid, 0, 1);
                break;
            case MODE_AUTO:
                if(!autobrew_routine_tick(&autobrew_plan)){
                    heater_pid.K.f = PID_GAIN_F;
                    binary_output_put(&solenoid, 0, 1);
                    if(autobrew_plan.state.pump_setting_changed){
                        phasecontrol_set_duty_cycle(&pump, autobrew_plan.state.pump_setting);
                    }
                } else {
                    heater_pid.K.f = 0;
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

    _state.boiler.temperature = lmt01_read(&thermo);
}

static void espresso_machine_update_leds(){
    if(_state.switches.ac_switch){
        binary_output_put(
            &leds, 0, 
            _state.switches.ac_switch);
        binary_output_put(
            &leds, 1, 
            _state.switches.ac_switch 
            && lmt01_read_float(&thermo) - heater_pid.setpoint < 2.5 
            && lmt01_read_float(&thermo) - heater_pid.setpoint > -2.5);
        binary_output_put(
            &leds, 2, 
            _state.switches.ac_switch 
            && !_state.switches.pump_switch 
            && nau7802_at_val_mg(&scale, settings[MS_WEIGHT_DOSE_DG]*100));
    } else {
        binary_output_mask(&leds, _state.settings_viewer_mask);
    }
}

static void espresso_machine_autobrew_setup(){
    uint32_t preinf_ramp_dur;
    uint32_t preinf_on_dur;
    uint8_t preinf_pwr = settings[MS_PWR_PREINF_I8];
    if(settings[MS_TIME_RAMP_DS] <= settings[MS_TIME_PREINF_ON_DS]){
        preinf_ramp_dur = settings[MS_TIME_RAMP_DS]*100000UL;
        preinf_on_dur = (settings[MS_TIME_PREINF_ON_DS]-settings[MS_TIME_RAMP_DS])*100000UL;
    } else {
        preinf_ramp_dur = settings[MS_TIME_PREINF_ON_DS]*100000UL;
        preinf_on_dur = 0;
    }

    uint32_t brew_ramp_dur;
    uint32_t brew_on_dur;
    uint8_t brew_pwr = settings[MS_PWR_BREW_I8];
    if(settings[MS_TIME_RAMP_DS] <= settings[MS_TIME_TIMEOUT_S]){
        brew_ramp_dur = settings[MS_TIME_RAMP_DS]*100000UL;
        brew_on_dur = (settings[MS_TIME_TIMEOUT_S]*10-settings[MS_TIME_RAMP_DS])*100000UL;
    } else {
        brew_ramp_dur = settings[MS_TIME_TIMEOUT_S]*1000000UL;
        brew_on_dur = 0;
    }

    uint32_t preinf_off_time = settings[MS_TIME_PREINF_OFF_DS]*100000UL;

    autobrew_leg_setup_function_call(&(autobrew_legs[0]),0, &zero_scale);
    autobrew_leg_setup_linear_power(&(autobrew_legs[1]), 60,         preinf_pwr, preinf_ramp_dur, NULL);
    autobrew_leg_setup_linear_power(&(autobrew_legs[2]), preinf_pwr, preinf_pwr, preinf_on_dur,   NULL);
    autobrew_leg_setup_linear_power(&(autobrew_legs[3]), 0,          0,          preinf_off_time, NULL);
    autobrew_leg_setup_linear_power(&(autobrew_legs[4]), 60,         brew_pwr,   brew_ramp_dur,   &scale_at_output);
    autobrew_leg_setup_linear_power(&(autobrew_legs[5]), brew_pwr,   brew_pwr,   brew_on_dur,     &scale_at_output);
    autobrew_routine_setup(&autobrew_plan, autobrew_legs, 6);
}

/** 
 * \brief Setup the local UI file structure.
*/
static void espresso_machine_setup_local_ui(){
    local_ui_folder_tree_init(&settings_modifier, &folder_root, "RaspberryLatte");
    local_ui_add_subfolder(&folder_root,                    &folder_settings, "Settings", NULL);
    local_ui_add_subfolder(&folder_settings,                &folder_settings_temp, "Temperatures", NULL);
    local_ui_add_subfolder(&folder_settings_temp,           &folder_settings_temp_brew, "Brew", &update_machine_setting);
    local_ui_add_subfolder(&folder_settings_temp,           &folder_settings_temp_hot, "Hot", &update_machine_setting);
    local_ui_add_subfolder(&folder_settings_temp,           &folder_settings_temp_steam, "Steam", &update_machine_setting);
    local_ui_add_subfolder(&folder_settings,                &folder_settings_weight, "Weights", NULL);
    local_ui_add_subfolder(&folder_settings_weight,         &folder_settings_weight_dose, "Dose", &update_machine_setting);
    local_ui_add_subfolder(&folder_settings_weight,         &folder_settings_weight_yield, "Yield", &update_machine_setting);
    local_ui_add_subfolder(&folder_settings,                &folder_settings_more, "More", NULL);
    local_ui_add_subfolder(&folder_settings_more,           &folder_settings_more_power, "Power", NULL);
    local_ui_add_subfolder(&folder_settings_more_power,     &folder_settings_more_power_brew, "Brew", &update_machine_setting);
    local_ui_add_subfolder(&folder_settings_more_power,     &folder_settings_more_power_hot, "Hot", &update_machine_setting);
    local_ui_add_subfolder(&folder_settings_more,           &folder_settings_more_preinfuse, "Preinfuse", NULL);
    local_ui_add_subfolder(&folder_settings_more_preinfuse, &folder_settings_more_preinfuse_on_time, "On Time", &update_machine_setting);
    local_ui_add_subfolder(&folder_settings_more_preinfuse, &folder_settings_more_preinfuse_on_power, "On Power", &update_machine_setting);
    local_ui_add_subfolder(&folder_settings_more_preinfuse, &folder_settings_more_preinfuse_off_time, "Off Time", &update_machine_setting);
    local_ui_add_subfolder(&folder_settings_more,           &folder_settings_more_misc, "Misc", NULL);
    local_ui_add_subfolder(&folder_settings_more_misc,      &folder_settings_more_misc_timeout, "Timeout", &update_machine_setting);
    local_ui_add_subfolder(&folder_settings_more_misc,      &folder_settings_more_misc_ramp_time, "Ramp Time", &update_machine_setting);
    local_ui_add_subfolder(&folder_root,                    &folder_presets, "Presets", NULL);
    local_ui_add_subfolder(&folder_presets,                 &folder_presets_profile_a, "Presets 1-3", NULL);
    local_ui_add_subfolder(&folder_presets_profile_a,       &folder_presets_profile_a_0, "Preset 1", &save_load_machine_presets);
    local_ui_add_subfolder(&folder_presets_profile_a,       &folder_presets_profile_a_1, "Preset 2", &save_load_machine_presets);
    local_ui_add_subfolder(&folder_presets_profile_a,       &folder_presets_profile_a_2, "Preset 3", &save_load_machine_presets);
    local_ui_add_subfolder(&folder_presets,                 &folder_presets_profile_b, "Presets 4-6", NULL);
    local_ui_add_subfolder(&folder_presets_profile_b,       &folder_presets_profile_b_0, "Preset 4", &save_load_machine_presets);
    local_ui_add_subfolder(&folder_presets_profile_b,       &folder_presets_profile_b_1, "Preset 5", &save_load_machine_presets);
    local_ui_add_subfolder(&folder_presets_profile_b,       &folder_presets_profile_b_2, "Preset 6", &save_load_machine_presets);
    local_ui_add_subfolder(&folder_presets,                 &folder_presets_profile_c, "Presets 7-9", NULL);
    local_ui_add_subfolder(&folder_presets_profile_c,       &folder_presets_profile_c_0, "Preset 7", &save_load_machine_presets);
    local_ui_add_subfolder(&folder_presets_profile_c,       &folder_presets_profile_c_1, "Preset 8", &save_load_machine_presets);
    local_ui_add_subfolder(&folder_presets_profile_c,       &folder_presets_profile_c_2, "Preset 9", &save_load_machine_presets);
}

int espresso_machine_setup(espresso_machine_viewer * state_viewer){
    if(state_viewer != NULL){
        *state_viewer = &_state;
    }

    i2c_bus_setup(bus, 100000, I2C_SCL_PIN, I2C_SDA_PIN);

    mb85_fram_setup(&mem, bus, 0x00, NULL);

    settings = machine_settings_setup(&mem);

    // Setup heater as a slow_pwm object
    slow_pwm_setup(&heater, HEATER_PWM_PIN, 1260, 64);
    heater_pid.K.p = PID_GAIN_P;
    heater_pid.K.i = PID_GAIN_I; 
    heater_pid.K.d = PID_GAIN_D;
    heater_pid.K.f = 0;
    heater_pid.min_time_between_ticks_ms = 100;
    heater_pid.sensor = &read_boiler_thermo;
    heater_pid.sensor_feedforward = &read_scale_flowrate;
    heater_pid.plant = &apply_boiler_input;
    heater_pid.setpoint = 0;
    pid_init(&heater_pid, 0, 150, 1000);

    // Setup the pressure sensor
    analog_input_setup(&pressure_sensor, PRESSURE_SENSOR_PIN);

    // Setup the LED binary output
    const uint8_t led_pins[3] = {LED0_PIN, LED1_PIN, LED2_PIN};
    binary_output_setup(&leds, led_pins, 3);

    // Setup the binary inputs for pump switch and mode dial.
    const uint8_t pump_switch_gpio = PUMP_SWITCH_PIN;
    const uint8_t mode_select_gpio[2] = {DIAL_A_PIN, DIAL_B_PIN};

    binary_input_setup(&pump_switch, 1, &pump_switch_gpio, BINARY_INPUT_PULL_UP, 10000, false, false);
    binary_input_setup(&mode_dial, 2, mode_select_gpio, BINARY_INPUT_PULL_UP, 75000, false, true);

    // Setup phase control
    phasecontrol_setup(&pump,PHASECONTROL_0CROSS_PIN,PHASECONTROL_OUT_PIN,PHASECONTROL_0CROSS_SHIFT,ZEROCROSS_EVENT_RISING);

    // Setup solenoid as a binary output
    uint8_t solenoid_pin [1] = {SOLENOID_PIN};
    binary_output_setup(&solenoid, solenoid_pin, 1);

    // Setup nau7802 and flowrate tracker
    nau7802_setup(&scale, bus, SCALE_CONVERSION_MG);
    discrete_derivative_init(&scale_flowrate, 250);

    // Setup thermometer
    lmt01_setup(&thermo, 0, LMT01_DATA_PIN);

    // Autobrew setup
    espresso_machine_autobrew_setup();
    espresso_machine_setup_local_ui();

    espresso_machine_update_state();

    machine_settings_print();
    return 0;
}

void espresso_machine_tick(){
    espresso_machine_update_state();
    espresso_machine_update_settings();
    espresso_machine_update_boiler();
    espresso_machine_update_pump();
    espresso_machine_update_leds();
}
