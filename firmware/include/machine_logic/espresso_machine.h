/**
 * \defgroup espresso_machine Espresso Machine Main
 * \ingroup machine_logic
 * \brief The functions and logic responsible for the operation of a single boiler espresso machine.
 * 
 * This is the core of the RaspberryLatte firmware. It handles the setup of all the drivers, tracks
 * the machine states, and updates its outputs accordingly. The logic is broken into 5 main parts.
 * -# Update state: Monitor the states of the machine switches and update parameters accordingly.
 * -# Update settings: Update machine settings using the \ref machine_settings.
 * -# Update boiler: Set the boiler's output and read its current temperature.
 * -# Update pump: Set the pumps power setting according to the switch state and/or autobrew routine.
 * -# Update LEDs: Set the LEDs to display the correct information given the machine context. 
 * 
 * Each of these 5 steps are run each time ::espresso_machine_tick is called. 
 * 
 * \{
 * \file
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Espresso Machine header
 * \version 0.1
 * \date 2022-08-16
 */

#ifndef ESPRESSO_MACHINE_H
#define ESPRESSO_MACHINE_H
#include "pico/stdlib.h"

/** \brief The different modes of the espresso machine corresponding to the 4P dial */
enum espresso_machine_modes {MODE_STEAM, MODE_HOT, MODE_MANUAL, MODE_AUTO};

/**
 * \brief The current state of the two switches and mode dial on espresso machine
 */
typedef struct {
    bool ac_switch;             /**< \brief Flag indicating if the AC power is off or on. */
    bool pump_switch;           /**< \brief Flag indicating if the pump switch is off or on. */
    uint8_t mode_dial;          /**< \brief Current value of the mode dial (0-3). */
    int8_t ac_switch_changed;   /**< \brief 1 if ac switch was turned on, -1 if turned off, 0 if no change. */
    int8_t pump_switch_changed; /**< \brief 1 if pump switch was turned on, -1 if turned off, 0 if no change. */
    int8_t mode_dial_changed;   /**< \brief 1 if mode dial was increased, -1 if decreased, 0 if no change. */
} espresso_machine_switch_state;

/**
 * \brief The current state of an espresso machine boiler under PID control
 */
typedef struct {
    uint16_t setpoint;    /**< \brief Current setpoint of the boiler. */
    int16_t temperature;  /**< \brief Current temperature of the boiler. */
    uint8_t power_level;  /**< \brief Current power level applied to the boiler. */
    uint16_t error_sum;   /**< \brief Current error sum of the boiler's controller. */
    uint16_t error_slope; /**< \brief Current error slope of the boiler's controller. */
} espresso_machine_boiler_state;

/**
 * \brief The current state of an espresso machine pump.
 */
typedef struct {
    uint8_t power_level; /**< \brief Power being applied to the pump. */
    bool pump_lock;      /**< \brief Flag indicating if the pump is locked. */
    int8_t brew_leg;     /**< \brief The current leg of the autobrew routine.*/
    float flowrate_ml_s; /**< \brief The rate the water is pumping in ml/s. */
    float pressure_bar;  /**< \brief The pressure of the pump in bar. */
} espresso_machine_pump_state;

/**
 * \brief The current state of a scale.
 */
typedef struct {
    int32_t val_mg; /**< \brief Value of the scale in mg. */
} espresso_machine_scale_state;

/**
 * \brief The full state of a single boiler espresso machine.
 */
typedef struct {
    espresso_machine_switch_state switches; /**< \brief State of espresso machine switches */
    espresso_machine_boiler_state boiler;   /**< \brief State of espresso machine boiler */
    espresso_machine_pump_state pump;       /**< \brief State of espresso machine pump */
    espresso_machine_scale_state scale;     /**< \brief State of espresso machine scale */
} espresso_machine_state;

/** \brief Struct designed to hold the espresso machine state and expose it to outside functions */
typedef const espresso_machine_state * espresso_machine_viewer;

/**
 * \brief Setup the espresso machine and save a pointer to an accessible state viewer.
 * 
 * \param state_viewer Pointer to an espresso_machine_viewer. This is saved so the machine state can be passed out to caller.
 * 
 * \returns 0 if successful, 1 on failure.
 */
int espresso_machine_setup(espresso_machine_viewer * state_viewer);

/**
 * \brief Tick the espresso machine.
 * 
 * This takes the following steps:
 * -# Read the switch state
 * -# Update the machine settings
 * -# Update the boiler
 * -# Update the pump
 * -# Update the LEDs
 * These functions populate the state_viewer passed into the setup function.
 */
void espresso_machine_tick();
#endif
/** \} */