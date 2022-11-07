#ifndef MACHINE_SETTINGS_H
#define MACHINE_SETTINGS_H

#include "pico/stdlib.h"
#include "mb85_fram.h"

typedef int16_t machine_setting;
typedef const machine_setting* machine_settings;

/** \brief Enumerated setting IDs. Used to read and set the various parameters */
typedef enum {
    MS_TIME_PREINF_ON_DS = 0, /**< The duration of the preinfuse on time in s/10 */
    MS_TIME_PREINF_OFF_DS,    /**< The duration of the preinfuse off time in s/10 */
    MS_TIME_TIMEOUT_S,        /**< The maximum duration of the autobrew routine in s */
    MS_TIME_RAMP_DS,          /**< The duration of the power ramp in s/10 */
    MS_WEIGHT_DOSE_DG,        /**< The weight of the grounds used in g/10 */
    MS_WEIGHT_YIELD_DG,       /**< The weight of espresso output in g/10 */
    MS_TEMP_BREW_DC,          /**< The tempurature for brewing in C/10 */
    MS_TEMP_HOT_DC,           /**< The tempurature for hot water in C/10 */
    MS_TEMP_STEAM_DC,         /**< The tempurature for steam in C/10 */
    MS_PWR_PREINF_I8,         /**< The preinfuse power in [0,127] */
    MS_PWR_BREW_I8,           /**< The brew power in [0,127] */
    MS_PWR_HOT_I8,            /**< The hot water power in [0,127] */
    MS_COUNT                  /**< The last element in the enum. Used for counting. */
} machine_setting_id;

/** \brief Initalize the settings and attach to memory device. 
 * \param mem A pointer to an initalized mb85_fram structure.
 * \returns A const pointer to the global settings structure or NULL on error. 
*/
machine_settings machine_settings_setup(mb85_fram * mem);

/** \brief Get const pointer to internal settings structure. 
 * \returns Pointer to internal settings structure or NULL if not setup.
*/
machine_settings machine_settings_aquire();

/**
 * \brief Save the current internal settings structure as the indicated ID.
 * 
 * \param profile A value from 0 to 8 indicating the profile to save to.
 * 
 * \returns PICO_ERROR_NONE if successfull. Else error code. 
*/
int machine_settings_save_profile(uint8_t profile);

/**
 * \brief Load the idicated profile int the internal settings structure.
 * 
 * If any value in the profile is unsafe, they will all be set to default values.
 * 
 * \param profile A value from 0 to 8 indicating the profile to load.
 * 
 * \returns PICO_ERROR_NONE if successfull. Else error code. 
*/
int machine_settings_load_profile(uint8_t profile);

/**
 * \brief Sets the parameter matching p_id to val. If val is out of range,
 * it is clipped to be within the parameter's bounds.
 * 
 * \param p_id ID of parameter that should be updated. 
 * \param val New value of the parameter in the correct units.
 * 
 * \returns PICO_ERROR_INVALID_ARG if p_id is out of range. Else PICO_ERROR_NONE.
*/
int machine_settings_set(machine_setting_id p_id, machine_setting val);

int machine_settings_print();
#endif