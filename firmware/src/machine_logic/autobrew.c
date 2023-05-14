/**
 * \ingroup autobrew
 * 
 * \file autobrew.c
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Local UI source
 * \version 1.0
 * \date 2022-12-05
*/
#include "machine_logic/autobrew.h"

#include "pico/time.h"
#include <stdlib.h>

#include "utils/macros.h"

typedef struct _autobrew_leg {
    int8_t   mapping_id;                             /**< Which of the configured mappings to use. -1 is straight mapping. */
    uint16_t setpoint_start;                         /**< Setpoint at start of leg. */
    uint16_t setpoint_end;                           /**< Setpoint at end of leg. */
    uint16_t timeout_ds;                             /**< Maximum duration of the leg in deciseconds. */
    uint8_t  setup_flags;                            /**< Any setup functions. */
    uint16_t trigger_vals[AUTOBREW_TRIGGER_MAX_NUM]; /**< Trigger values for the end of a leg (0 -> no trigger). */
} autobrew_leg;

static autobrew_trigger _triggers[AUTOBREW_TRIGGER_MAX_NUM];
static autobrew_mapping _mappings[AUTOBREW_MAPPING_MAX_NUM];
static autobrew_setup_fun _setup_funs[AUTOBREW_SETUP_FUN_MAX_NUM];
static uint8_t _num_triggers = 0;
static uint8_t _num_mappings = 0;
static uint8_t _num_setup_funs = 0;

static autobrew_leg _routine[AUTOBREW_LEG_MAX_NUM];
static uint8_t _num_legs = 0;
static uint8_t _current_leg = 0;
static absolute_time_t _leg_end_time;

/**
 * \brief Initialize all leg struct values to 0 or NULL.
 * \param leg_idx Index of leg that will be cleared
 */
static void _autobrew_clear_leg_struct(uint8_t leg_idx){
    _routine[leg_idx].setpoint_start = 0; 
    _routine[leg_idx].setpoint_end = 0;
    _routine[leg_idx].timeout_ds = 0; 
    _routine[leg_idx].setup_flags = 0;
    for (uint8_t i = 0; i < AUTOBREW_TRIGGER_MAX_NUM; i++){
        _routine[leg_idx].trigger_vals[i] = 0;
    }
}

/**
 * \brief Computes the current setpoint of the leg
*/
static uint16_t _autobrew_get_current_setpoint(){
    const int64_t t_remaining_ms = absolute_time_diff_us(get_absolute_time(), _leg_end_time)/1000;
    if (t_remaining_ms < 0){
        return _routine[_current_leg].setpoint_end;
    } else {
        const int16_t c = _routine[_current_leg].setpoint_end - _routine[_current_leg].setpoint_start;
        return _routine[_current_leg].setpoint_start + c -
        (c * t_remaining_ms)/(100*(int)_routine[_current_leg].timeout_ds);
    }
}

/**
 * \brief Takes a leg and updates the state based on the current time. 
 * 
 * The state includes the pump value, if the pump has changed since the last tick, and if the leg has finished.
 * 
 * \param leg Pointer to leg that will be ticked.
 * \param state Pointer to autobrew_state var where the state of the autobrew leg should be stored after the tick.
 */
static bool _autobrew_leg_tick(autobrew_state * state){
    state->leg_id = _current_leg;

    // If at end of routine, just set state to known value and exit
    if(_current_leg == _num_legs){ 
        state->pump_setting = 0;
        state->pump_setting_changed = false;
        state->leg_id = _current_leg;
        return false;
    }

    autobrew_leg * cl = &_routine[_current_leg];

    // First time leg has ticked?
    if(is_nil_time(_leg_end_time)){ 
        // Save timeout for leg
        _leg_end_time = make_timeout_time_ms(10*(uint32_t)cl->timeout_ds);
        // Run all startup functions for leg
        for(uint8_t i = 0; i < AUTOBREW_SETUP_FUN_MAX_NUM; i++){
            if(cl->setup_flags & (1 << i)) _setup_funs[i]();
        }
    }

    // Check if leg has finished
    state->finished = absolute_time_diff_us(get_absolute_time(), _leg_end_time) >= 0;
    for(uint8_t i = 0; i < _num_triggers; i++){
        if(state->finished){
            _leg_end_time = nil_time;
            _current_leg += 1;
            state->leg_id = _current_leg;
            state->pump_setting = 0;
            state->pump_setting_changed = true; // make sure 0 is applied
            return true;
        }
        if(cl->trigger_vals[i] > 0 ){
            state->finished = _triggers[i](cl->trigger_vals[i]);
        }
    }

    // Get new pump setting.
    const uint16_t setpoint = _autobrew_get_current_setpoint();
    const uint8_t new_setting = (cl->mapping_id == -1 ? setpoint : _mappings[cl->mapping_id](setpoint));
    state->pump_setting_changed = (new_setting != state->pump_setting);
    state->pump_setting = new_setting;

    return false;
}

void autobrew_routine_setup(){
    _num_legs = 0;
    _current_leg = 0;
    _leg_end_time = nil_time;
}

uint8_t autobrew_register_trigger(autobrew_trigger trigger){
    if(_num_triggers == AUTOBREW_TRIGGER_MAX_NUM) return PICO_ERROR_GENERIC;
    _triggers[_num_triggers] = trigger;
    _num_triggers += 1;
    return _num_triggers - 1;
}

uint8_t autobrew_register_mapping(autobrew_mapping mapping){
    if(_num_mappings == AUTOBREW_MAPPING_MAX_NUM) return PICO_ERROR_GENERIC;
    _mappings[_num_mappings] = mapping;
    _num_mappings += 1;
    return _num_mappings - 1;
}

uint8_t autobrew_register_setup_fun(autobrew_setup_fun setup_fun){
    if(_num_setup_funs == AUTOBREW_SETUP_FUN_MAX_NUM) return PICO_ERROR_GENERIC;
    _setup_funs[_num_setup_funs] = setup_fun;
    _num_setup_funs += 1;
    return _num_setup_funs - 1;
}

uint8_t autobrew_add_leg(int8_t mapping_id, uint16_t setpoint_start, uint16_t setpoint_end, uint16_t timeout_ds){
    assert(_num_legs < AUTOBREW_LEG_MAX_NUM);
    _autobrew_clear_leg_struct(_num_legs);
    _routine[_num_legs].mapping_id = mapping_id;
    _routine[_num_legs].setpoint_start = setpoint_start;
    _routine[_num_legs].setpoint_end = setpoint_end;
    _routine[_num_legs].timeout_ds  = timeout_ds;
    _num_legs += 1;
    return (_num_legs-1);
}

void autobrew_configure_leg_trigger(uint8_t leg_id, uint8_t trigger_id, uint16_t trigger_val){
    assert(trigger_id < AUTOBREW_TRIGGER_MAX_NUM);
    _routine[leg_id].trigger_vals[trigger_id] = trigger_val;
}

void autobrew_configure_leg_setup_fun(uint8_t leg_id, uint8_t setup_fun_id, bool enable){
    assert(setup_fun_id < AUTOBREW_SETUP_FUN_MAX_NUM);
    if(enable){
        _routine[leg_id].setup_flags = _routine[leg_id].setup_flags | (1<<setup_fun_id);
    } else {
        _routine[leg_id].setup_flags = _routine[leg_id].setup_flags & (~(1<<setup_fun_id));
    }
}

bool autobrew_routine_tick(autobrew_state * s){
    // tick until reaching a leg with work left to do or end of routine
    while(_autobrew_leg_tick(s));
    return s->finished;
}

void autobrew_reset(){
    autobrew_routine_setup();
}
