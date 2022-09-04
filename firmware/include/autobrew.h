/**
 * \file
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Header defining API for autobrew routines.
 * \version 0.1
 * \date 2022-08-16
 */

#ifndef AUTOBREW_H
#define AUTOBREW_H
#include "pico/stdlib.h"

/** \brief Function prototype that will be called during a function call leg */
typedef int (*autobrew_fun)();

/** \brief Function prototype for an ending trigger. Used to terminate a leg prior to the timeout */
typedef bool (*autobrew_trigger)();

/**
 * \brief Stuct for tracking the state of an autobrew leg and/or routine.
 * 
 * This struct is populated by tick calls (either leg or routine versions). 
 */
typedef struct {
    uint8_t pump_setting;       /**< The value that the autobrew routine will set the pump to*/
    bool pump_setting_changed;  /**< Indicates if the pump_setting changed since the last tick*/
    bool finished;              /**< Indicates if the leg/routine has completed*/
} autobrew_state;

/** 
 * \brief Stuct defining a single leg of the autobrew routine. 
 * 
 * Each leg is defined by a (possibly variable) pump power setting, and an end 
 * condition. The pump power level can either be constant, or change linearly
 * over the legs duration. The end condition must always contain a timeout
 * length in microseconds but can also be triggered with a function returning 
 * true when some user defined condition is met. An exception to this is a leg 
 * used to call some void function. Such a leg is only ticked once and, during 
 * that tick, the function is called and the starting power is returned.
 *
 * \todo Add PID regulation option
 */
typedef struct {
    uint8_t pump_pwr_start;   /**< Starting or constant power */
    uint8_t pump_pwr_delta;   /**<  Change in power over leg. If constant, set to 0.*/
    uint32_t timeout_us;      /**<  Maximum duration of the leg in microseconds. Actual length may be shorter if trigger is used.*/
    autobrew_fun fun;         /**<  Void function to call if FUNCTION_CALL leg*/
    autobrew_trigger trigger; /**<  Function returning bool triggering some action*/

    uint64_t _end_time_us;    /**<  End time of leg. Set the first time the leg is ticked*/
} autobrew_leg;

/**
 * \brief A full autobrew routine comprised of an array of autobrew legs.
 */
typedef struct {
    autobrew_state state; /**< The current state of the routine. Polulated during calles to autobrew_routine_tick*/
    uint8_t _num_legs;    /**< The number of legs in the routine.*/
    uint8_t _cur_leg;     /**< The index of the current leg. Incremented when the current leg finishes.*/
    autobrew_leg * _legs; /**< Array of _num_legs autobrew legs.*/
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
 * If there is a trigger function, this value may never be realized.
 * \param timeout_us Time in microseconds from the first tick to the end of the leg if never triggered.
 * \param trigger Trigger function that returns true when some end condition is met (e.g. scale hits 30g).
 * If NULL, only the timeout_us is used and the leg is a timed leg.
 */
void autobrew_leg_setup_linear_power(autobrew_leg * leg, uint8_t pump_starting_pwr, 
                                    uint8_t pump_ending_pwr, uint32_t timeout_us,
                                    autobrew_trigger trigger);

/**
 * \brief Setup the autobrew_routine * r with the passed in legs.
 * 
 * \param r Pointer to autobrew_routine struct that represents the routine.
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