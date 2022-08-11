#ifndef _AUTOBREW_H
#define _AUTOBREW_H
#include "pico/stdlib.h"

typedef void (*autobrew_fun)();
typedef bool (*autobrew_trigger)();

/**
 * \brief Stuct defining a single leg of the autobrew routine. Each leg is defined by a (possibly variable)
 * pump power setting, and an end condition. The pump power level can either be constant, or change linearly
 * over the legs duration (TODO: ADD PID REGULATION OPTION). The end condition must always contain a timeout
 * length in microseconds but can also be triggered with a function returning true when some user defined
 * condition is met. An exception to this is a leg used to call some void function. Such a leg is only ticked
 * once and, during that tick, the function is called and the starting power is returned.
 * 
 * Based on what values are set, different things may happen. First, if .fun != NULL, then only .pump_pwr_start
 * and .fun are used. After the first tick, .fun is called and .pump_pwr_start is returned as the pump power. 
 * Else, the leg will begin to change from .pump_pwr_start to .pump_pwr_start+.pump_pwr_delta over the 
 * .timeout_us. If .trigger!=NULL, then at each tick we check if .trigger()==true and, if so, the leg is 
 * finished.
 */
typedef struct {
    uint8_t pump_pwr_start; // Starting or constant power
    uint8_t pump_pwr_delta; // Change in power over leg. If constant, set to 0.

    uint32_t timeout_us;  // Maximum duration of the leg in microseconds. Actual length may be shorter if trigger is used.

    autobrew_fun fun;   // Void function to call if FUNCTION_CALL leg
    autobrew_trigger trigger; // Function returning bool triggering some action

    uint64_t _end_time_us;  // End time of leg. Set the first time the leg is ticked
} autobrew_leg;

typedef struct {
    uint8_t pump_setting;
    bool pump_setting_changed;
    bool finished;
} autobrew_state;

typedef struct {
    autobrew_state state;
    uint8_t _num_legs;
    uint8_t _cur_leg;
    autobrew_leg * _legs;
} autobrew_routine;

int autobrew_leg_setup_function_call(autobrew_leg * leg, uint8_t pump_pwr, autobrew_fun fun);
int autobrew_leg_setup_linear_power(autobrew_leg * leg, uint8_t pump_starting_pwr, 
                                    uint8_t pump_ending_power, uint32_t duration_us,
                                    autobrew_trigger trigger);

int autobrew_routine_setup(autobrew_routine * r, autobrew_leg * legs, uint8_t num_legs);

int autobrew_routine_tick(autobrew_routine * r);

int autobrew_routine_reset(autobrew_routine * r);
#endif