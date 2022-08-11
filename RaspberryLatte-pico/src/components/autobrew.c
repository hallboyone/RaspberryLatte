#include "pico/time.h"

#include "autobrew.h"

static void clear_leg_struct(autobrew_leg * leg){
    leg->pump_pwr_start = 0; 
    leg->pump_pwr_delta = 0;
    leg->timeout_us = 0; 
    leg->_end_time_us = 0;
    leg->fun = NULL;
    leg->trigger = NULL;
}

static void reset_leg(autobrew_leg * leg){
    leg->_end_time_us = 0;
}

static int autobrew_leg_tick(autobrew_leg * leg, autobrew_state * state){
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

        // Compute end condition by .trigger (if avalible) or timeout.
        if(leg->trigger != NULL){
            state->finished = (leg->trigger() || leg->_end_time_us <= time_us_64());
        } else {
            state->finished = (leg->_end_time_us <= time_us_64());
        }
    }
    return 1;
}

int autobrew_leg_setup_function_call(autobrew_leg * leg, uint8_t pump_pwr, autobrew_fun fun){
    clear_leg_struct(leg);
    leg->fun = fun;
    leg->pump_pwr_start = pump_pwr;
}

int autobrew_leg_setup_linear_power(autobrew_leg * leg, uint8_t pump_starting_pwr, 
                                    uint8_t pump_ending_power, uint32_t timeout_us,
                                    autobrew_trigger trigger){
    clear_leg_struct(leg);
    leg->pump_pwr_start = pump_starting_pwr;
    leg->pump_pwr_delta = pump_ending_power - pump_starting_pwr;
    leg->trigger = trigger;
    leg->timeout_us = timeout_us;
}

int autobrew_routine_setup(autobrew_routine * r, autobrew_leg * legs, uint8_t num_legs){
    r->_legs = legs;
    r->_num_legs = num_legs;
    r->_cur_leg = 0;
    r->state.pump_setting = 0;
    r->state.pump_setting_changed = false;
    r->state.finished = false;
}

int autobrew_routine_tick(autobrew_routine * r){
    if(r->state.finished) return 1;
    autobrew_leg_tick(&(r->_legs[r->_cur_leg]), &(r->state));
    if(r->state.finished){
        r->_cur_leg += 1;
        r->state.finished = (r->_cur_leg==r->_num_legs);
    }
    return r->state.finished;
}

int autobrew_routine_reset(autobrew_routine * r){
    r->_cur_leg = 0;
    r->state.pump_setting = 0;
    r->state.pump_setting_changed = false;
    r->state.finished = false;
    for(uint i = 0; i<r->_num_legs; i++){
        reset_leg(&(r->_legs[i]));
    }
}
