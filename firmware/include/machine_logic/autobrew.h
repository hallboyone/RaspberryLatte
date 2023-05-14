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
 * \version 1.0
 * \date 2022-12-05
 */

#ifndef AUTOBREW_H
#define AUTOBREW_H
#include "pico/stdlib.h"
#include "utils/pid.h"

#define AUTOBREW_LEG_MAX_NUM 16

#define AUTOBREW_SETUP_FUN_MAX_NUM  3
#define AUTOBREW_TRIGGER_MAX_NUM    3
#define AUTOBREW_MAPPING_MAX_NUM    3

/** 
 * \brief Function prototype for routines that should run at the start of a leg (e.g. reset PID controller) 
 */
typedef void (*autobrew_setup_fun)();

/** 
 * \brief Function prototype for a function that converts a setpoint to a pump power.
 * 
 * An example use case would be calling a PID controller object.
 */
typedef uint8_t (*autobrew_mapping)(float);

/** 
 * \brief Function prototype for an ending trigger. 
 * 
 * Used to terminate a leg prior to the timeout. 
 */
typedef bool (*autobrew_trigger)(uint8_t);

/**
 * \brief Struct for tracking the state of an autobrew leg and/or routine.
 * 
 * This struct is updated internally using the autobrew_routine_tick function. 
 */
typedef struct {
    uint8_t pump_setting;       /**< \brief The percent power that the autobrew routine has set the pump to. */
    uint8_t leg_id;             /**< \brief The current leg of the autobrew routine. */
    bool pump_setting_changed;  /**< \brief Indicates if the pump_setting changed since the last tick. */
    bool finished;              /**< \brief Indicates if the leg/routine has completed. */
} autobrew_state;

/** \brief Resets the autobrew library to a known state. */
void autobrew_setup();

/** 
 * \brief Registers a trigger function that can be used to end a leg.
 * \param trigger Pointer to a trigger function.
 * \returns The ID of the trigger. 
 */
uint8_t autobrew_register_trigger(autobrew_trigger trigger);

/** 
 * \brief Registers a mapping function that can be used to map setpoints to pump powers.
 * \param mapping Pointer to a mapping function.
 * \returns The ID of the mapping. 
 */
uint8_t autobrew_register_mapping(autobrew_mapping mapping);

/** 
 * \brief Registers a setup function that can be called at the start of a leg.
 * \param trigger Pointer to a setup function.
 * \returns The ID of the setup function. 
 */
uint8_t autobrew_register_setup_fun(autobrew_setup_fun setup_fun);

/** 
 * \brief Create an autobrew leg with the minimal required elements.
 * \param mapping_id The ID of a previously registered mapping. -1 to use setpoint directly.
 * \param setpoint_start The value of the setpoint at the start of the leg.
 * \param setpoint_end The value of the setpoint at the leg's duration.
 * \param timeout_ds The timeout duration of the autobrew leg.
 * \returns The ID of the leg that was created. 
 */
uint8_t autobrew_add_leg(int8_t mapping_id, uint16_t setpoint_start, uint16_t setpoint_end, uint16_t timeout_ds);

/** 
 * \brief Adds an end trigger to specific leg ID.
 * \param leg_id The id of the leg to add the trigger to.
 * \param trigger_id The ID of a previously registered trigger.
 * \param trigger_val The value to pass to the trigger function. 0 to disable trigger.
 */
void autobrew_configure_leg_trigger(uint8_t leg_id, uint8_t trigger_id, uint16_t trigger_val);

/** 
 * \brief Adds an setup function to specific leg ID.
 * \param leg_id The id of the leg to add the setup function to.
 * \param setup_fun_id The ID of a previously registered setup function.
 * \param enable Indicates if the setup function should be enabled or not. 
 */
void autobrew_configure_leg_setup_fun(uint8_t leg_id, uint8_t setup_fun_id, bool enable);

//void autobrew_configure_leg_packed(uint64_t packed_leg);

/**
 * \brief Run a single tick of the autobrew routine. 
 * 
 * The resulting pump setting can be accessed within s->pump_setting.
 * 
 * \param s Pointer to state structure where the results will be stored.
 * 
 * \return True if the routine has finished. False otherwise.
 */
bool autobrew_routine_tick(autobrew_state * s);

/**
 * \brief Reset the internal fields of r so that the routine is restarted at the next tick.
 * 
 * \param r Pointer to autobrew_routine that will be reset.
 */
void autobrew_routine_reset();
#endif

/** \} */