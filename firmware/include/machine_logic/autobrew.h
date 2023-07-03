/**
 * \defgroup autobrew Autobrew Library
 * \version 1.0
 * 
 * \brief Provides tools for running multi-leg brew procedures.
 * 
 * The Autobrew Library manages brew procedures made up by one or more legs. Each leg begins with 
 * 0 - AUTOBREW_SETUP_FUN_MAX_NUM setup functions that run one time at the start of the leg. Each 
 * leg runs until either a timeout has been reached or until any of 0 - AUTOBREW_TRIGGER_MAX_NUM 
 * trigger functions returns true. While running, the legs map a linearly changing setpoint to a 
 * pump power using mapping functions or 1-to-1 logic if NULL (i.e. the setpoint is the pump power). 
 * 
 * To use, the library must first be initalized. Then, the legs of the routine are added sequentially 
 * using the autobrew_add_leg function. Setup functions and triggers can be added to legs as needed. 
 * Once all the legs have been added and configured, calling autobrew_routine_tick repeatedly followed 
 * by autobrew_current_power will move through the routine according to the configured logic and 
 * return the corresponding pump power. At the end of the routine, calling autobrew_reset will 
 * restore the library to the starting state and the next autobrew routine can be run.
 * 
 * \ingroup machine_logic
 * \{
 * \file
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Header defining API for autobrew routines.
 * \version 1.0
 * \date 2023-05-19
 */

#ifndef AUTOBREW_H
#define AUTOBREW_H
#include "pico/stdlib.h"
#include "utils/pid.h"

#define AUTOBREW_LEG_MAX_NUM 16

#define AUTOBREW_SETUP_FUN_MAX_NUM 3
#define AUTOBREW_TRIGGER_MAX_NUM   3

#define AUTOBREW_PUMP_POWER_MAX 100

/** 
 * \brief Function prototype for routines that should run at the start of a leg (e.g. reset PID controller) 
 */
typedef void (*autobrew_setup_fun)();

/** 
 * \brief Function prototype for a function that converts a setpoint to a pump power.
 * 
 * Function converts a setpoint to a pump power between 0 and AUTOBREW_PUMP_POWER_MAX.
 * An example use case would be calling a PID controller object.
 */
typedef uint8_t (*autobrew_mapping)(uint16_t);

/** 
 * \brief Function prototype for an ending trigger. 
 * 
 * Used to terminate a leg prior to the timeout.
 * 
 * \return True if leg should end. Else returns false. 
 */
typedef bool (*autobrew_trigger)(uint16_t);

/** 
 * \brief Initializes the autobrew library. 
 */
void autobrew_init();

/** 
 * \brief Create an autobrew leg with the minimal required elements.
 * 
 * Calls to this function create sequential legs in the routine. Setup legs in the order they should be run.
 * 
 * \param mapping Mapping function. NULL to use setpoint directly.
 * \param setpoint_start The value of the setpoint at the start of the leg.
 * \param setpoint_end The value of the setpoint at the leg's duration.
 * \param timeout_ms The timeout duration of the autobrew leg.
 * 
 * \returns The ID of the leg that was created. 
 */
uint8_t autobrew_add_leg(autobrew_mapping mapping, uint16_t setpoint_start, uint16_t setpoint_end, uint16_t timeout_ms);

/** 
 * \brief Adds an end trigger to specific leg ID.
 * \param leg_id The id of the leg to add the trigger to.
 * \param trigger A trigger function.
 * \param trigger_data The value to pass to the trigger function.
 */
void autobrew_leg_add_trigger(uint8_t leg_id, autobrew_trigger trigger, uint16_t trigger_data);

/** 
 * \brief Adds an setup function to specific leg ID.
 * \param leg_id The id of the leg to add the setup function to.
 * \param setup_fun A setup function to call when leg is started.
 */
void autobrew_leg_add_setup_fun(uint8_t leg_id, autobrew_setup_fun setup_fun);

/**
 * \brief Run a single tick of the autobrew routine. 
 * 
 * The resulting pump setting can be accessed with autobrew_pump_power().
 * 
 * \return True if the routine has finished. False otherwise.
 */
bool autobrew_routine_tick();

/**
 * \brief Returns the current pump power according to the autobrew routine.
 * 
 * \return The current pump power between 0 and  AUTOBREW_PUMP_POWER_MAX.
 */
uint8_t autobrew_pump_power();

/**
 * \brief Checks if pump power was changed with the last tick.
 * 
 * \return True if pump power changed with the last tick. Else false. 
 */
bool autobrew_pump_changed();

/**
 * \brief Returns the index of the current leg.
 * 
 * \return The index of the current leg. If routine has finished, then -1 is returned.
 */
int8_t autobrew_current_leg();

/**
 * \brief Checks if autobrew routine has finished. 
 * 
 * \return True if routine has finished. False if it has not finished. 
 */
bool autobrew_finished();

/**
 * \brief Reset internal fields so that the routine is restarted at the next tick.
 */
void autobrew_reset();
#endif

/** \} */