#include "espresso_machine.h"

static bool pump_lock = false;
static uint8_t current_mode = 0;
static uint autobrew_target_output_mg = 30000;
static uint autobrew_bean_dose_mg = 16000;

/**
 * \brief Helper function for the PID controller. Returns the boiler temp in C.
 */
float read_boiler_thermo(){
    return lmt01_read_float(&thermo);
}

/**
 * \brief Helper function for the PID controller. Applies an input to the boiler heater.
 */
void apply_boiler_input(float u){
    slow_pwm_set_float_duty(&heater, u);
}

/** 
 * \brief Returns true if scale is greater than or equal to the passed in value. 
 */
bool scale_at_val(int val_mg){
    return nau7802_read_mg() >= val_mg;
}

/** 
 * \brief Returns true if scale is greater than or equal to the current output. 
 */
bool scale_at_output(){
    return scale_at_val(autobrew_target_output_mg);
}

void update_setpoint(){
    if(phasecontrol_is_ac_hot(&pump)){
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

void update_pump_lock(){
    if(binary_input_read(&mode_dial) != current_mode){
        nau7802_zero();
        pump_lock = true;
        current_mode = binary_input_read(&mode_dial);
    }
    if(pump_lock){
        if(!binary_input_read(&pump_switch)){
            pump_lock = false;
        }
    }
}

void update_pump(){
    update_pump_lock();
    if(!binary_input_read(&pump_switch)){
        autobrew_routine_reset(&autobrew_plan);
        phasecontrol_set_duty_cycle(&pump, 0);
        binary_output_put(&solenoid, 0, 0);
    } else if (pump_lock){
        phasecontrol_set_duty_cycle(&pump, 0);
        binary_output_put(&solenoid, 0, 0);
    } else {
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

void update_leds(){
    bool ac_on = phasecontrol_is_ac_hot(&pump);
    binary_output_put(&leds, 0, ac_on);
    binary_output_put(&leds, 1, ac_on && lmt01_read_float(&thermo) - heater_pid.setpoint < 2.5 && lmt01_read_float(&thermo) - heater_pid.setpoint > -2.5);
    binary_output_put(&leds, 2, ac_on && !binary_input_read(&pump_switch) && scale_at_val(autobrew_bean_dose_mg));
}