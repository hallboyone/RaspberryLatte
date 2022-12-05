/**
 * \ingroup autobrew
 * 
 * \file autobrew.c
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Local UI source
 * \version 0.2
 * \date 2022-12-05
*/

#include "pico/time.h"

#include "autobrew.h"
#include <stdlib.h>

typedef struct _autobrew_leg {
    uint8_t pump_pwr_start;   /**< \brief Pump power at start of leg. */
    int8_t pump_pwr_delta;    /**< \brief Change in power over leg. If constant, set to 0. */
    uint32_t timeout_us;      /**< \brief Maximum duration of the leg in microseconds. Actual length may be shorter if trigger is used.*/
    autobrew_fun fun;         /**< \brief Void function to call if FUNCTION_CALL leg. */
    autobrew_trigger trigger; /**< \brief Function used to trigger the end of a leg. */

    uint64_t _end_time_us;    /**< \brief End time of leg. Set the first time the leg is ticked*/
} autobrew_leg;

/**
 * \brief Initialize all leg struct values to 0 or NULL.
 * 
 * \param leg Pointer to leg that will be cleared
 */
static void _autobrew_clear_leg_struct(autobrew_leg * leg){
    leg->pump_pwr_start = 0; 
    leg->pump_pwr_delta = 0;
    leg->timeout_us = 0; 
    leg->_end_time_us = 0;
    leg->fun = NULL;
    leg->trigger = NULL;
}

/**
 * \brief After a leg is ticked once, an end time is assigned internally. This function clears that end
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
        state->pump_setting = leg->pump_pwr_start;
        state->pump_setting_changed = true;
        state->finished = true;
    } else { // Linear power leg
        // If first tick, set the endtime and indicate pump changed. Otherwise, indicate pump unchanged.
        if(leg->_end_time_us == 0){ 
            leg->_end_time_us = time_us_64() + leg->timeout_us;
            state->pump_setting_changed = true;
        } else {
            state->pump_setting_changed = false;
        }
        // If power has linear change, compute current value and indicate change.
        if(leg->pump_pwr_delta != 0){ // Set pump power
            state->pump_setting_changed = true;
            float percent_complete = 1;
            if(leg->_end_time_us > time_us_64()){
                percent_complete = 1 - (leg->_end_time_us - time_us_64())/(float)leg->timeout_us;
            }
            state->pump_setting = leg->pump_pwr_start + percent_complete*leg->pump_pwr_delta;
        } else {
            state->pump_setting = leg->pump_pwr_start;
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
    r->_legs[leg_idx].pump_pwr_start = pump_pwr;

    return PICO_ERROR_NONE;
}

int autobrew_setup_linear_power_leg(autobrew_routine * r, uint8_t leg_idx, uint8_t pump_starting_pwr, 
                                uint8_t pump_ending_pwr, uint32_t timeout_us,
                                autobrew_trigger trigger){
    if(r->_num_legs <= leg_idx) return PICO_ERROR_INVALID_ARG;

    _autobrew_clear_leg_struct(&(r->_legs[leg_idx]));
    r->_legs[leg_idx].pump_pwr_start = pump_starting_pwr;
    r->_legs[leg_idx].pump_pwr_delta = pump_ending_pwr - pump_starting_pwr;
    r->_legs[leg_idx].trigger = trigger;
    r->_legs[leg_idx].timeout_us = timeout_us;

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
