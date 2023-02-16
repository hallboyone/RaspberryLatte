/**
 * \defgroup autobrew Autobrew Library
 * \version 0.2
 * 
 * \brief Provides tools for running multi-leg brew procedures.
 * 
 * The Autobrew Library is built around \ref autobrew_routine structures containing
 * an array of internally managed \ref autobrew_leg structures. After setting up a routine
 * and configuring it's legs, it can be ticked through. This increments through each leg of
 * the routine, updating the pump power accordingly and calling any required callback 
 * functions. Each leg is either ended after it times out, a trigger function returns true,
 * or it call's its own callback function. 
 * 
 * \todo Add PID regulation option
 * 
 * \ingroup machine_logic
 * \{
 * \file
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Header defining API for autobrew routines.
 * \version 0.2
 * \date 2022-12-05
 */

#ifndef AUTOBREW_H
#define AUTOBREW_H
#include "pico/stdlib.h"

/** \brief Function prototype that will be called during a function call leg. */
typedef int (*autobrew_fun)();

/** \brief Function prototype for an ending trigger. Used to terminate a leg prior to the timeout. */
typedef bool (*autobrew_trigger)();

/**
 * \brief Struct for tracking the state of an autobrew leg and/or routine.
 * 
 * This struct is updated by tick calls (either leg or routine versions). 
 */
typedef struct {
    uint8_t pump_setting;       /**< \brief The percent power that the autobrew routine has set the pump to. */
    bool pump_setting_changed;  /**< \brief Indicates if the pump_setting changed since the last tick. */
    bool finished;              /**< \brief Indicates if the leg/routine has completed. */
} autobrew_state;

/** 
 * \brief Struct defining a single leg of the autobrew routine. 
 * 
 * Each leg is defined by a (possibly variable) pump power setting, and an end 
 * condition. The pump power level can either be constant, or change linearly
 * over the leg's duration. The end condition must always contain a timeout
 * length in microseconds but can also be triggered with a function returning 
 * true when some user defined condition is met. An exception to this is a leg 
 * used to call a void function. Such a leg is only ticked once and, during that 
 * tick, the function is called and the starting power is returned.
 */
typedef struct _autobrew_leg autobrew_leg;

/**
 * \brief A full autobrew routine comprised of an array of autobrew legs.
 */
typedef struct {
    autobrew_state state; /**< \brief The current state of the routine. Populated during calls to autobrew_routine_tick*/
    uint8_t _num_legs;    /**< \brief The number of legs in the routine.*/
    uint8_t _cur_leg;     /**< \brief The index of the current leg. Incremented when the current leg finishes.*/
    autobrew_leg * _legs; /**< \brief Array of _num_legs autobrew legs.*/
} autobrew_routine;

/**
 * \brief Sets up the required fields in an autobrew_leg within a autobrew_routine to create a 
 * function call leg.
 * 
 * \param r Previously setup autobrew_routine structure with the leg that will be setup.
 * \param leg_idx The index of the leg to be setup (0 indexed).
 * \param pump_pwr The percent power that will be returned after the leg's only tick.
 * \param fun The function that will be called during the leg's only tick.
 * 
 * \returns PICO_ERROR_NONE if successful. Else, PICO_ERROR_INVALID_ARG if leg_idx is out of range.
 */
int autobrew_setup_function_call_leg(autobrew_routine * r, uint8_t leg_idx, uint8_t pump_pwr, autobrew_fun fun);

/**
 * \brief Sets up the required fields in an \ref autobrew_leg within a autobrew_routine to create a 
 * linear power leg. 
 * 
 * Leg may have an optional end trigger. If not desired, pass \ref NULL as the \ref trigger.
 * 
 * \param r Previously setup autobrew_routine structure with the leg that will be setup.
 * \param leg_idx The index of the leg to be setup (0 indexed).
 * \param pump_starting_pwr Percent power at the start of the leg.
 * \param pump_ending_pwr Percent power at the end of the leg. Note this power is reached at the end of the timeout.
 * If there is a trigger function, this value may never be realized.
 * \param timeout_us Time in microseconds from the first tick to the end of the leg if never triggered.
 * \param trigger Trigger function that returns true when some end condition is met (e.g. scale hits 30g).
 * If NULL, only the timeout_us is used and the leg is a timed leg.
 * 
 * \returns PICO_ERROR_NONE if successful. Else, PICO_ERROR_INVALID_ARG if leg_idx is out of range.
 */
int autobrew_setup_linear_power_leg(autobrew_routine * r, uint8_t leg_idx, uint8_t pump_starting_pwr, 
                                    uint8_t pump_ending_pwr, uint32_t timeout_us, autobrew_trigger trigger);

/**
 * \brief Setup the autobrew_routine * r with \ref num_legs empty legs.
 * 
 * \note Should only be called once per \ref autobrew_routine. Otherwise memory will be leaked.
 * 
 * \param r Pointer to autobrew_routine struct that represents the routine.
 * \param num_legs Number of legs in routine.
 */
void autobrew_routine_setup(autobrew_routine * r, uint8_t num_legs);

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

/** \} */