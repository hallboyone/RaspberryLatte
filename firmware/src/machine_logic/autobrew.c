/**
 * \ingroup autobrew
 * 
 * \file autobrew.c
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Local UI source
 * \version 0.2
 * \date 2022-12-05
*/
#include "machine_logic/autobrew.h"

#include "pico/time.h"
#include <stdlib.h>

#include "utils/macros.h"
typedef struct _autobrew_leg {
    float pump_setpoint_start;    /**< \brief Setpoint at start of leg. */
    float pump_setpoint_delta;    /**< \brief Change in Setpoint over leg. If constant, set to 0. */
    uint32_t timeout_us;          /**< \brief Maximum duration of the leg in microseconds. Actual length may be shorter if trigger is used.*/
    autobrew_fun fun;             /**< \brief Void function to call if FUNCTION_CALL leg. */
    autobrew_trigger trigger;     /**< \brief Function used to trigger the end of a leg. */
    autobrew_get_power get_power; /**< \brief Converts floating point setpoint to pump power. If NULL, then setpoint is power. */
    uint64_t _end_time_us;        /**< \brief End time of leg. Set the first time the leg is ticked*/
} autobrew_leg;

/**
 * \brief Initialize all leg struct values to 0 or NULL.
 * 
 * \param leg Pointer to leg that will be cleared
 */
static void _autobrew_clear_leg_struct(autobrew_leg * leg){
    leg->pump_setpoint_start = 0; 
    leg->pump_setpoint_delta = 0;
    leg->timeout_us = 0; 
    leg->_end_time_us = 0;
    leg->fun = NULL;
    leg->trigger = NULL;
}

/**
 * \brief Computes the current setpoint given a leg with a endtime, timeout, start power, and delta power.
*/
static float _autobrew_get_current_setpoint(autobrew_leg * leg){
    float current_setpoint = leg->pump_setpoint_start;
    if(leg->pump_setpoint_delta != 0){
        float percent_complete = 1;
        if(leg->_end_time_us > time_us_64()){
            percent_complete = 1 - (leg->_end_time_us - time_us_64())/(float)leg->timeout_us;
        }
        current_setpoint = leg->pump_setpoint_start + percent_complete*leg->pump_setpoint_delta;
    }
    return current_setpoint;
}

/**
 * \brief Reset function leg. 
 * 
 * After a leg is ticked once, an end time is assigned internally. This function clears that end
 * time so it's as if the leg has never been called.
 * 
 * \param leg Pointer to leg that will be reset
 */
static void _autobrew_reset_leg(autobrew_leg * leg){
    leg->_end_time_us = 0;
}

/**
 * \brief Takes a leg and updates the state based on the current time. The state includes the pump value,
 * if the pump has changed since the last tick, and if the leg has finished.
 * 
 * \param leg Pointer to leg that will be ticked.
 * \param state Pointer to autobrew_state var where the state of the autobrew leg should be stored after the tick.
 */
static void _autobrew_leg_tick(autobrew_leg * leg, autobrew_state * state){
    if(leg->fun != NULL){ // Function call leg
        leg->fun();
        state->pump_setting = leg->pump_setpoint_start;
        state->pump_setting_changed = true;
        state->finished = true;
    } else { // Linear setpoint leg
        if(leg->_end_time_us == 0){ // If first tick, set the endtime and indicate pump changed.
            leg->_end_time_us = time_us_64() + leg->timeout_us;
            state->pump_setting_changed = true;
        } else { // Pump only changes after the first tick if setpoint is variable or indirect
            state->pump_setting_changed = (leg->pump_setpoint_delta != 0) || leg->get_power != NULL;
        }

        // Compute the current setpoint, accounting for linear changes
        float current_setpoint = _autobrew_get_current_setpoint(leg);

        if(leg->get_power == NULL){ // No controller. Setpoint is raw controller power
            state->pump_setting = CLAMP(current_setpoint, 0, 100);
        } else { // External function generates pump_setting given setpoint
            state->pump_setting = CLAMP(leg->get_power(current_setpoint), 0, 100);

        }

        // Compute end condition by .trigger (if available) or timeout.
        if(leg->trigger != NULL){
            state->finished = (leg->trigger() || leg->_end_time_us <= time_us_64());
        } else {
            state->finished = (leg->_end_time_us <= time_us_64());
        }
    }
}

int autobrew_setup_function_call_leg(autobrew_routine * r, uint8_t leg_idx, uint8_t pump_pwr, autobrew_fun fun){
    if(r->_num_legs <= leg_idx) return PICO_ERROR_INVALID_ARG;
    
    _autobrew_clear_leg_struct(&(r->_legs[leg_idx]));
    r->_legs[leg_idx].fun = fun;
    r->_legs[leg_idx].pump_setpoint_start = pump_pwr;

    return PICO_ERROR_NONE;
}

int autobrew_setup_linear_setpoint_leg(autobrew_routine * r, uint8_t leg_idx, float pump_starting_setpoint, 
                                    float pump_ending_setpoint, autobrew_get_power power_computer, uint32_t timeout_us, 
                                    autobrew_trigger trigger){
    if(r->_num_legs <= leg_idx) return PICO_ERROR_INVALID_ARG;

    _autobrew_clear_leg_struct(&(r->_legs[leg_idx]));
    r->_legs[leg_idx].pump_setpoint_start = pump_starting_setpoint;
    r->_legs[leg_idx].pump_setpoint_delta = pump_ending_setpoint - pump_starting_setpoint;
    r->_legs[leg_idx].trigger = trigger;
    r->_legs[leg_idx].timeout_us = timeout_us;
    r->_legs[leg_idx].get_power = power_computer;
    return PICO_ERROR_NONE;
}

void autobrew_routine_setup(autobrew_routine * r, uint8_t num_legs){
    r->_num_legs = num_legs;
    r->_cur_leg = 0;
    r->state.pump_setting = 0;
    r->state.pump_setting_changed = false;
    r->state.finished = false;

    // Allocate and clear legs
    r->_legs = (autobrew_leg*)malloc(num_legs * sizeof(autobrew_leg));
    for(uint i = 0; i<r->_num_legs; i++){
        _autobrew_clear_leg_struct(&(r->_legs[i]));
    }
}

bool autobrew_routine_tick(autobrew_routine * r){
    if(r->state.finished){
        // Routine already finished. Nothing to do.
        r->state.pump_setting = 0;
        return true;
    } else {
        _autobrew_leg_tick(&(r->_legs[r->_cur_leg]), &(r->state));
        if(r->state.finished){
            r->_cur_leg += 1;
            if(r->_cur_leg==r->_num_legs){
                // Finished last leg. Set pump to 0.
                r->state.pump_setting = 0;
            } else {
                // More legs to go so routine is not finished
                r->state.finished = false;
            }
        }
        return r->state.finished;
    }
}

void autobrew_routine_reset(autobrew_routine * r){
    r->_cur_leg = 0;
    r->state.pump_setting = 0;
    r->state.pump_setting_changed = false;
    r->state.finished = false;
    for(uint i = 0; i<r->_num_legs; i++){
        _autobrew_reset_leg(&(r->_legs[i]));
    }
}
