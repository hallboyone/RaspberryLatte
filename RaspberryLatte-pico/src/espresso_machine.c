#include "espresso_machine.h"

static int32_t scale_origin = 0;
bool pump_lock = false;
uint8_t current_mode = 0;

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
 * \brief Function that handles the conversion from the raw nau7802 values into mg
 */
int scale_read(){
    int32_t scale_val;
    nau7802_read(&scale_val);
    return 0.152710615479*(float)(scale_val - scale_origin); // in mg
}

/**
 * \brief Record the current scale value and subtract that from future readings.
 */
void scale_zero(){
    do {
        nau7802_read(&scale_origin);
    } while (scale_origin==0);
}

/** 
 * \brief Returns true if scale is greater than or equal to the passed in value. 
 */
bool scale_at_val(int val_mg){
    return scale_read() >= val_mg;
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
        zero_scale();
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
    binary_output_put(&leds, 0, phasecontrol_is_ac_hot(&pump));
    binary_output_put(&leds, 1, lmt01_read_float(&thermo) - heater_pid.setpoint < 2.5 && lmt01_read_float(&thermo) - heater_pid.setpoint > -2.5);
    binary_output_put(&leds, 2, !binary_input_read(&pump_switch) && scale_at_val(autobrew_bean_dose_mg));
}