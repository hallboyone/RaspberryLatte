#ifndef MACHINE_SETTINGS_H
#define MACHINE_SETTINGS_H

#include "pico/stdlib.h"
#include "mb85_fram.h"

typedef int16_t machine_setting;
typedef machine_setting* temp_dC;     // Temperature value. Divide by 10 to get C
typedef machine_setting* duration_ds; // Duration length. Divide by 10 to get seconds
typedef machine_setting* duration_s;  // Duration length in seconds
typedef machine_setting* power_per;   // Power level as percentage
typedef machine_setting* weight_dg;   // Weight value. Divide by 10 to get g

typedef struct {
    temp_dC temp;
} machine_settings_steam;

typedef struct {
    temp_dC   temp;
    power_per power;
} machine_settings_hot;

typedef struct {
    temp_dC   temp;
    power_per power;
    weight_dg dose;
    weight_dg yield;
} machine_settings_brew;

typedef struct {
    duration_ds preinf_on_time;   /**< Length of time to soak puck during pre-infuse */
    duration_ds preinf_off_time;  /**< Length of time to let puck soak during pre-infuse */
    power_per   preinf_power;     /**< The power of the pre-infuse routine */
    duration_ds preinf_ramp_time; /**< The time, in ds of the linear ramp to target power*/
    duration_s  timeout;          /**< The length of time to attempt to reach yield*/
} machine_settings_auto;

typedef struct {
    machine_settings_auto  autobrew;
    machine_settings_brew  brew;
    machine_settings_hot   hot;
    machine_settings_steam steam;
    uint8_t ui_mask;
} machine_settings;

// /** Pointer to internal settings field. Allows access to current settings 
//  * but prohibits external changes. */
// const machine_settings * ms;

/** \brief Initialize the settings and attach to memory device. 
 * \param mem A pointer to an initalized mb85_fram structure.
 * \returns A const pointer to the global settings structure or NULL on error. 
*/
const machine_settings * machine_settings_setup(mb85_fram * mem);

/**
 * \brief Navigates the internal setting's tree and updates values accordingly
 * 
 * \param reset Flag indicating if the internal UI should be reset.
 * \param select Flag indicating if select condition is met
 * \param val Value of selector
 * \return int 
 */
int machine_settings_update(bool reset, bool select, uint8_t val);

int machine_settings_print();
#endif