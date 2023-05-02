/**
 * \defgroup thermal_runaway_watcher Thermal Runaway Watcher
 * \ingroup utils
 * \brief State machine to watch for thermal runaways in the system.
 * 
 * There are three errors that this state machine looks for.
 * 1) When heating/cooling, the temperature does not move toward the setpoint?
 * 2) When converged, the temperature deviates far from the setpoint?
 * 3) Consecutive temperature readings vary widely?
 * 
 * \{
 * 
 * \file thermal_runaway_watcher.h
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Thermal Runaway Watcher header
 * \version 0.1
 * \date 2023-04-28
 */

#ifndef THERMAL_RUNAWAY_WATCHER_H
#define THERMAL_RUNAWAY_WATCHER_H

// Uncomment to compile with testing functions
//#define THERMAL_RUNAWAY_WATCHER_TESTS

/** \brief Enumerated list of possible thermal states. Errors are less than 0. */
typedef enum {TRS_ERROR_DIVERGED = -3, TRS_ERROR_FAILED_TO_CONVERGE, TRS_ERROR_LARGE_TEMP_JUMP, TRS_OFF, TRS_HEATING, TRS_COOLING, TRS_CONVERGED} thermal_runaway_state;

/** \brief Object defining a state-machine tracking the thermal state of an espresso machine. */
typedef struct thermal_runaway_watcher_s* thermal_runaway_watcher;

/** \brief Configure a new thermal_runaway_watcher object.
 * 
 * Sets the internal parameters which determine how sensitive the watcher is to thermal runaways.
 * 
 * \param temp_max_change The maximum the temp is allowed to change in a single step.
 * \param temp_convergence_tol The range around the setpoint that indicates convergence.
 * \param temp_divergence_limit The range around the setpoint that a converged value must stay within.
 * \param min_temp_change_heat The minimum increase in temperature over each window required when heating.
 * \param min_temp_change_cool The minimum decrease in temperature over each window required when cooling.
 * \param min_temp_change_time_ms The length of the window to observe the minimum changes in temperature. 
 * 
 * \returns A newly created thermal_runaway_watcher object.
*/
thermal_runaway_watcher thermal_runaway_watcher_setup(uint16_t temp_max_change,
    uint16_t temp_convergence_tol, uint16_t temp_divergence_limit, uint16_t min_temp_change_heat, 
    uint16_t min_temp_change_cool, uint32_t min_temp_change_time_ms);

/** \brief Update the internal state of the thermal_runaway_watcher based on the current setpoint and temp.
 * 
 * \param trw The thermal_runaway_watcher object to tick.
 * \param setpoint The current setpoint of the system.
 * \param temp The current temp of the system.
 * 
 * \returns The new state of the system.
*/
thermal_runaway_state thermal_runaway_watcher_tick(thermal_runaway_watcher trw, uint16_t setpoint, int16_t temp);

/** \brief Returns the latest state of the system.
 * \param trw The thermal_runaway_watcher object to examine.
 * \returns The new state of the system.
*/
thermal_runaway_state thermal_runaway_watcher_state(thermal_runaway_watcher trw);

/** \brief Destroy the thermal_runaway_watcher object. 
 * \param trw The thermal_runaway_watcher object to destroy.
*/
void thermal_runaway_watcher_deinit(thermal_runaway_watcher trw);

#ifdef THERMAL_RUNAWAY_WATCHER_TESTS
/** \brief Run a sequence of tests on the library.
 * 
 * Compiled by defining THERMAL_RUNAWAY_WATCHER_TESTS in header.
*/
void thermal_runaway_watcher_test();
#endif

#endif
/** \}*/