#include "pico/time.h"

#include "autobrew.h"

static void clear_leg_struct(autobrew_leg * leg){
    leg->pump_pwr_start = 0; 
    leg->pump_pwr_delta = 0;
    leg->duration_us = 0; 
    leg->end_time_us = 0;
    leg->fun = NULL;
    leg->trigger = NULL;
}

static void reset_leg(autobrew_leg * leg){
    leg->end_time_us = 0;
}

static int autobrew_tick_function_call_leg(autobrew_leg * leg, autobrew_state * state){
    if(leg->leg_type != FUNCTION_CALL) return 0;
    leg->fun();
    state->pump_setting = leg->pump_pwr_start;
    state->pump_setting_changed = true;
    state->finished = true;
    return 1;
}

static int autobrew_tick_ramp_leg(autobrew_leg * leg, autobrew_state * state){
    if(leg->leg_type != RAMP) return 0;
    state->pump_setting_changed = true;
    if(leg->end_time_us == 0){
        leg->end_time_us = time_us_64() + leg->duration_us;
    }
    float percent_complete = 1 - (leg->end_time_us - time_us_64())/(float)leg->duration_us;
    state->pump_setting = leg->pump_pwr_start + percent_complete*leg->pump_pwr_delta;
    state->finished = (leg->end_time_us <= time_us_64());
    return 1;
}

static int autobrew_tick_constant_timed_leg(autobrew_leg * leg, autobrew_state * state){
    if(leg->leg_type != CONSTANT_TIMED) return 0;
    
    if(leg->end_time_us == 0){
        leg->end_time_us = time_us_64() + leg->duration_us;
        state->pump_setting_changed = true;
    } else {
        state->pump_setting_changed = false;
    }
    state->pump_setting = leg->pump_pwr_start;
    state->finished = (leg->end_time_us <= time_us_64());
    return 1;
}

static int autobrew_tick_constant_triggered_leg(autobrew_leg * leg, autobrew_state * state){
    if(leg->leg_type != CONSTANT_TRIGGERED) return 0;
    
    if(leg->end_time_us == 0){
        leg->end_time_us = time_us_64() + leg->duration_us;
        state->pump_setting_changed = true;
    } else {
        state->pump_setting_changed = false;
    }
    state->pump_setting = leg->pump_pwr_start;
    state->finished = (leg->trigger() || leg->end_time_us <= time_us_64());
    return 1;
}

int autobrew_setup_function_call_leg(autobrew_leg * leg, autobrew_fun fun, uint8_t pwr){
    clear_leg_struct(leg);
    leg->leg_type = FUNCTION_CALL;
    leg->fun = fun;
    leg->pump_pwr_start = pwr;
}

int autobrew_setup_ramp_leg(autobrew_leg * leg, uint8_t pump_starting_pwr, uint8_t pump_ending_power, uint32_t duration_us){
    clear_leg_struct(leg);
    leg->leg_type = RAMP;
    leg->pump_pwr_start = pump_starting_pwr;
    leg->pump_pwr_delta = pump_ending_power - pump_starting_pwr;
    leg->duration_us = duration_us;
}

int autobrew_setup_constant_timed_leg(autobrew_leg * leg, uint8_t pump_pwr, uint32_t duration_us){
    clear_leg_struct(leg);
    leg->leg_type = CONSTANT_TIMED;
    leg->pump_pwr_start = pump_pwr;
    leg->duration_us = duration_us;
}


int autobrew_setup_constant_triggered_leg(autobrew_leg * leg, uint8_t pump_pwr, autobrew_trigger trigger, uint32_t timeout_us){
    clear_leg_struct(leg);
    leg->leg_type = CONSTANT_TRIGGERED;
    leg->pump_pwr_start = pump_pwr;
    leg->trigger = trigger;
    leg->duration_us = timeout_us;
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
    switch(r->_legs[r->_cur_leg].leg_type){
        case FUNCTION_CALL:
            autobrew_tick_function_call_leg(&(r->_legs[r->_cur_leg]), &(r->state));
            break;
        case RAMP:
            autobrew_tick_ramp_leg(&(r->_legs[r->_cur_leg]), &(r->state));
            break;
        case CONSTANT_TIMED:
            autobrew_tick_constant_timed_leg(&(r->_legs[r->_cur_leg]), &(r->state));
            break;
        case CONSTANT_TRIGGERED:
            autobrew_tick_constant_triggered_leg(&(r->_legs[r->_cur_leg]), &(r->state));
            break;
    }
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
