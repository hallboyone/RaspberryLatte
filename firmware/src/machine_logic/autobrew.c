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
    autobrew_mapping mapping;                                  /**< Which of the configured mappings to use. -1 is straight mapping. */
    uint16_t setpoint_start;                                   /**< Setpoint at start of leg. */
    uint16_t setpoint_end;                                     /**< Setpoint at end of leg. */
    uint16_t timeout_ms;                                       /**< Maximum duration of the leg in milliseconds. */
    uint16_t trigger_data[AUTOBREW_TRIGGER_MAX_NUM];           /**< Trigger values for the end of a leg (0 -> no trigger). */
    autobrew_trigger triggers[AUTOBREW_TRIGGER_MAX_NUM];       /**< Trigger values for the end of a leg (0 -> no trigger). */
    autobrew_setup_fun setup_funs[AUTOBREW_SETUP_FUN_MAX_NUM]; /**< All the setup functions to run at start of leg. */
} autobrew_leg;

static autobrew_leg _routine[AUTOBREW_LEG_MAX_NUM]; /**< Array for storing all the configured legs of the routine. */
static uint8_t _num_legs = 0;                       /**< The number of legs that have been configured. */
static uint8_t _current_leg = 0;                    /**< The leg the routine is currently on. */
static absolute_time_t _leg_end_time;               /**< The timeout for the current leg. nil_time if leg has ended. */
static uint8_t _current_power;                      /**< The latest power computed in the routine. */
static bool _pump_changed;                          /**< Flag indicating if the pump has changed between ticks. */

/**
 * \brief Initialize all leg struct values to 0 or NULL.
 * \param leg_idx Index of leg that will be cleared
 */
static void _autobrew_clear_leg_struct(uint8_t leg_idx){
    _routine[leg_idx].setpoint_start = 0; 
    _routine[leg_idx].setpoint_end = 0;
    _routine[leg_idx].timeout_ms = 0; 
    for (uint8_t i = 0; i < AUTOBREW_TRIGGER_MAX_NUM; i++){
        _routine[leg_idx].trigger_data[i] = 0;
        _routine[leg_idx].triggers[i] = NULL;
    }
    for (uint8_t i = 0; i < AUTOBREW_SETUP_FUN_MAX_NUM; i++){
        _routine[leg_idx].setup_funs[i] = NULL;
    }
    _routine[leg_idx].mapping = NULL;
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
        (c * t_remaining_ms)/(_routine[_current_leg].timeout_ms);
    }
}

/**
 * \brief Takes a leg and updates the state based on the current time.
 * \return True if leg has ended. Else false. 
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
        _leg_end_time = make_timeout_time_ms(cl->timeout_ms);
        // Run all startup functions for leg
        for(uint8_t i = 0; i < AUTOBREW_SETUP_FUN_MAX_NUM; i++){
            if(cl->setup_funs[i] == NULL) break;
            cl->setup_funs[i]();
        }
    }

    // Check if leg has finished
    bool leg_finished = absolute_time_diff_us(get_absolute_time(), _leg_end_time) <= 0;
    for(uint8_t i = 0; i < AUTOBREW_TRIGGER_MAX_NUM; i++){
        if(leg_finished){
            _leg_end_time = nil_time;
            _current_leg += 1;
            _current_power = 0;
            return true;
        }
        if(cl->triggers[i] == NULL) break;
        leg_finished = cl->triggers[i](cl->trigger_data[i]) ;
    }

    // Get new pump setting.
    const uint16_t setpoint = _autobrew_get_current_setpoint();
    _current_power = (cl->mapping == NULL ? setpoint : cl->mapping(setpoint));
    assert(_current_power <= AUTOBREW_PUMP_POWER_MAX);

    return false;
}

void autobrew_init(){
    _num_legs = 0;
    autobrew_reset();
}

uint8_t autobrew_add_leg(autobrew_mapping mapping, uint16_t setpoint_start, uint16_t setpoint_end, uint16_t timeout_ms){
    assert(_num_legs < AUTOBREW_LEG_MAX_NUM);
    _autobrew_clear_leg_struct(_num_legs);
    _routine[_num_legs].mapping = mapping;
    _routine[_num_legs].setpoint_start = setpoint_start;
    _routine[_num_legs].setpoint_end = setpoint_end;
    _routine[_num_legs].timeout_ms  = timeout_ms;
    _num_legs += 1;
    return (_num_legs-1);
}

void autobrew_leg_add_trigger(uint8_t leg_id, autobrew_trigger trigger, uint16_t trigger_data){
    assert(leg_id < AUTOBREW_LEG_MAX_NUM);
    for(uint8_t i = 0; i < AUTOBREW_TRIGGER_MAX_NUM; i++){
        if(_routine[leg_id].triggers[i] == NULL){
            _routine[leg_id].triggers[i] = trigger;
            _routine[leg_id].trigger_data[i] = trigger_data;
            return;
        }
    }
    assert(false); // If we reach this line, we have no more trigger slots
}

void autobrew_leg_add_setup_fun(uint8_t leg_id, autobrew_setup_fun setup_fun){
    assert(leg_id < AUTOBREW_LEG_MAX_NUM);
    for(uint8_t i = 0; i < AUTOBREW_SETUP_FUN_MAX_NUM; i++){
        if(_routine[leg_id].setup_funs[i] == NULL){
            _routine[leg_id].setup_funs[i] = setup_fun;
            return;
        }
    }
    assert(false); // If we reach this line, we have no more setup function slots
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