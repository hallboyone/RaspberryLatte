/**
 * \ingroup autobrew
 * 
 * \file autobrew.c
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Local UI source
 * \version 1.0
 * \date 2023-05-19
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

static autobrew_leg _routine[AUTOBREW_LEG_MAX_NUM];
static uint8_t _num_legs = 0;
static uint8_t _current_leg = 0;
static absolute_time_t _leg_end_time;
static uint8_t _current_power;
static bool _pump_changed;

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
*/
static bool _autobrew_leg_tick(){
    // If at end of routine, just set state to known value and exit
    if(_current_leg == _num_legs){ 
        _current_power = 0;
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
    bool leg_finished = absolute_time_diff_us(get_absolute_time(), _leg_end_time) >= 0;
    for(uint8_t i = 0; i < AUTOBREW_TRIGGER_MAX_NUM; i++){
        if(leg_finished){
            _leg_end_time = nil_time;
            _current_leg += 1;
            _current_power = 0;
            return true;
        }
        if(cl->trigger_vals[i] > 0 && _triggers[i] != NULL){
            leg_finished = _triggers[i](cl->trigger_vals[i]);
        }
    }

    // Get new pump setting.
    const uint16_t setpoint = _autobrew_get_current_setpoint();
    _current_power = (cl->mapping_id == -1 ? setpoint : _mappings[cl->mapping_id](setpoint));
    assert(_current_power <= AUTOBREW_PUMP_POWER_MAX);

    return false;
}

void autobrew_init(){
    for(uint i = 0; i < AUTOBREW_TRIGGER_MAX_NUM; i++)   _triggers[i]   = NULL;
    for(uint i = 0; i < AUTOBREW_MAPPING_MAX_NUM; i++)   _mappings[i]   = NULL;
    for(uint i = 0; i < AUTOBREW_SETUP_FUN_MAX_NUM; i++) _setup_funs[i] = NULL;
    autobrew_clear_routine();
    autobrew_reset();
}

void autobrew_clear_routine(){
    _num_legs = 0;
}

void autobrew_register_trigger(uint8_t id, autobrew_trigger trigger){
    assert(id < AUTOBREW_TRIGGER_MAX_NUM && _triggers[id] == NULL);
    _triggers[id] = trigger;
}

void autobrew_register_mapping(uint8_t id, autobrew_mapping mapping){
    assert(id < AUTOBREW_MAPPING_MAX_NUM && _mappings[id] == NULL);
    _mappings[id] = mapping;
}

void autobrew_register_setup_fun(uint8_t id, autobrew_setup_fun setup_fun){
    assert(id < AUTOBREW_SETUP_FUN_MAX_NUM && _setup_funs[id] == NULL);
    _setup_funs[id] = setup_fun;
}

uint8_t autobrew_add_leg(int8_t mapping_id, uint16_t setpoint_start, uint16_t setpoint_end, uint16_t timeout_ds){
    assert(_num_legs < AUTOBREW_LEG_MAX_NUM);
    assert(mapping_id == -1 || (mapping_id < AUTOBREW_MAPPING_MAX_NUM && _mappings[mapping_id] != NULL));
    _autobrew_clear_leg_struct(_num_legs);
    _routine[_num_legs].mapping_id = mapping_id;
    _routine[_num_legs].setpoint_start = setpoint_start;
    _routine[_num_legs].setpoint_end = setpoint_end;
    _routine[_num_legs].timeout_ds  = timeout_ds;
    _num_legs += 1;
    return (_num_legs-1);
}

void autobrew_configure_leg_trigger(uint8_t leg_id, uint8_t trigger_id, uint16_t trigger_val){
    assert(_triggers[trigger_id] != NULL);
    _routine[leg_id].trigger_vals[trigger_id] = trigger_val;
}

void autobrew_configure_leg_setup_fun(uint8_t leg_id, uint8_t setup_fun_id, bool enable){
    assert(_setup_funs[setup_fun_id] != NULL);
    if(enable){
        _routine[leg_id].setup_flags = _routine[leg_id].setup_flags | (1<<setup_fun_id);
    } else {
        _routine[leg_id].setup_flags = _routine[leg_id].setup_flags & (~(1<<setup_fun_id));
    }
}

bool autobrew_routine_tick(){
    uint8_t previous_power = _current_power;
    // tick until reaching a leg with work left to do or end of routine
    while(_autobrew_leg_tick()) tight_loop_contents();

    _pump_changed = (_current_power!=previous_power);
    return autobrew_finished();
}

uint8_t autobrew_pump_power(){
    return _current_power;
}

bool autobrew_pump_changed(){
    return _pump_changed;
}

int8_t autobrew_current_leg(){
    return (autobrew_finished() ? -1 : _current_leg);
}

bool autobrew_finished(){
    return _current_leg == _num_legs;
}

void autobrew_reset(){
    _current_leg = 0;
    _leg_end_time = nil_time;
    _current_power = 0;
    _pump_changed = false;
}
