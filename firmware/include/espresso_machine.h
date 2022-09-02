/**
 * \file
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Header for interfacing with a single boiler espresso machine.
 * \version 0.1
 * \date 2022-08-16
 * 
 * Machine is assumed to have a power and pump switch, a mode dial, an output scale,
 * and other features.
 * 
 * \todo Add ability to customize machine configuration.
 */

#ifndef ESPRESSO_MACHINE_H
#define ESPRESSO_MACHINE_H
#include "pico/stdlib.h"

/** \brief The different modes of the espresso machine corrisponding to the 4P dial */
enum espresso_machine_modes {MODE_STEAM, MODE_HOT, MODE_MANUAL, MODE_AUTO};

/**
 * \brief The current state of the two switches and mode dial on espresso machine
 */
typedef struct {
    bool ac_switch;    /**< Flag indicating if the AC power is off or on. */
    bool pump_switch;  /**< Flag indicating if the pump switch is off or on. */
    uint8_t mode_dial; /**< Current value of the mode dial (0-3). */
} espresso_machine_switch_state;

/**
 * \brief The current state of an espresso machine boiler under PID control
 */
typedef struct {
    uint16_t setpoint;    /**< Current setpoint of the boiler. */
    int16_t tempurature;  /**< Current tempurature of the boiler. */
    uint8_t power_level;  /**< Current power level applied to the boiler. */
    uint16_t error_sum;   /**< Current error sum of the boiler's controller. */
    uint16_t error_slope; /**< Current error slope of the boiler's controller. */
} espresso_machine_boiler_state;

/**
 * \brief The current state of an espresso machine pump.
 */
typedef struct {
    uint8_t power_level; /**< Power being applied to the pump. */
    bool pump_lock;      /**< Flag indicating if the pump is locked. */
    int8_t brew_leg;     /**< The current leg of the autobrew routine.*/
} espresso_machine_pump_state;

/**
 * \brief The current state of a scale.
 */
typedef struct {
    int32_t val_mg; /**< Value of the scale in mg. */
} espresso_machine_scale_state;

/**
 * \brief The full state of a single boiler espresso machine.
 */
typedef struct {
    espresso_machine_switch_state switches; /**< State of espresso machine switches */
    espresso_machine_boiler_state boiler;   /**< State of espresso machine boiler */
    espresso_machine_pump_state pump;       /**< State of espresso machine pump */
    espresso_machine_scale_state scale;     /**< State of espresso machine scale */
} espresso_machine_state;

/** \brief Struct designed to hold the espresso machine state and expose it to outside functions */
typedef const espresso_machine_state * espresso_machine_viewer;

/**
 * \brief Setup the espresso machine and save a pointer to an accessable state viewer.
 * 
 * \param state_viewer Pointer to an espresso_machine_viewer. This is saved so the machine state can be passed out to caller.
 * 
 * \returns 0 if successfull, 1 on failure.
 */
int espresso_machine_setup(espresso_machine_viewer * state_viewer);

/**
 * \brief Tick the espresso machine.
 * 
 * This takes the following steps:
 * - Read the switch state
 * - Update the boiler
 * - Update the pump
 * - Update the LEDs
 * These functions populate the state_viewer passed into the setup function.
 */
void espresso_machine_tick();
#endif
