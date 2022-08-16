/**
 * \file
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Header defining API for autobrew routines.
 * \version 0.1
 * \date 2022-08-16
 */

#ifndef _AUTOBREW_H
#define _AUTOBREW_H
#include "pico/stdlib.h"

typedef void (*autobrew_fun)();
typedef bool (*autobrew_trigger)();

/** @brief Stuct defining a single leg of the autobrew routine. 
 * 
 * Each leg is defined by a (possibly variable)
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
typedef struct _autobrew_leg {
    uint8_t pump_pwr_start; /**< Starting or constant power */
    uint8_t pump_pwr_delta; /**<  Change in power over leg. If constant, set to 0.*/

    uint32_t timeout_us;  /**<  Maximum duration of the leg in microseconds. Actual length may be shorter if trigger is used.*/

    autobrew_fun fun;   /**<  Void function to call if FUNCTION_CALL leg*/
    autobrew_trigger trigger; /**<  Function returning bool triggering some action*/

    uint64_t _end_time_us;  /**<  End time of leg. Set the first time the leg is ticked*/
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

/**
 * \brief Helper function to assign the required fields in an autobrew_leg to create a function call leg.
 * 
 * \param leg Pointer to autobrew_leg struct that will be setup as a function call leg.
 * \param pump_pwr The power that will be returned after the leg's only tick.
 * \param fun The function that will be called during the leg's only tick.
 */
void autobrew_leg_setup_function_call(autobrew_leg * leg, uint8_t pump_pwr, autobrew_fun fun);

/**
 * \brief Helper function to assign the required fields in an autobrew_leg to create leg with linear
 * change in the power and an optional ending trigger.
 * 
 * \param leg Pointer to autobrew_leg struct that will be setup as a linear power leg.
 * \param pump_starting_pwr Power at the start of the leg.
 * \param pump_ending_pwr Power at the end of the leg. Note this power is reached at the end of the timeout.
 * If there is a trigger function, the pump_ending_pwr may never be realized.
 * \param timeout_us Time in microseconds from the first tick to the end of the leg if never triggered.
 * \param trigger Trigger function that returns true when some end condition is met (e.g. scale hits 30g).
 * If NULL, only the timeout_us is used and the leg is a timed leg.
 */
void autobrew_leg_setup_linear_power(autobrew_leg * leg, uint8_t pump_starting_pwr, 
                                    uint8_t pump_ending_power, uint32_t duration_us,
                                    autobrew_trigger trigger);

/**
 * \brief Setup the autobrew_routine * r with the passed in legs.
 * 
 * \param r Pointer to autobrew_routine struct where the required values are saved.
 * \param legs Pointer to array of legs defining the full autobrew routine. These should be setup using
 * the autobrew_leg_setup_function_call and autobrew_leg_setup_linear_power functions. This array must 
 * outlive the autobrew_routine.
 * \param num_legs Number of elements in legs array.
 */
void autobrew_routine_setup(autobrew_routine * r, autobrew_leg * legs, uint8_t num_legs);

/**
 * \brief Run a single tick of the autobrew routine. The resulting pump setting can be 
 * accessed within r.state.pump_setting
 * 
 * \param r Pointer to a previously setup autobrew_routine.
 * 
 * \return True if the routine has finished. False otherwise.
 */
bool autobrew_routine_tick(autobrew_routine * r);

/**
 * \brief Reset the internal fields of r so that the routine is restarted at the next tick.
 * 
 * \param r Pointer to autobrew_routine that will be reset.
 */
void autobrew_routine_reset(autobrew_routine * r);
#endif