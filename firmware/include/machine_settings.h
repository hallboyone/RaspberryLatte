#ifndef MACHINE_SETTINGS_H
#define MACHINE_SETTINGS_H

#define MACHINE_SETTINGS_STARTING_ADDRESS 0x00

#include "pico/stdlib.h"

#if defined(COUNTER_CULTURE_FORTY_SIX)
const float TEMP_SETPOINTS [4] = {140, 100, 93, 93};
#elif defined(COUNTER_CULTURE_SLOW_MOTION)
const float TEMP_SETPOINTS [4] = {140, 100, 95, 95};
#elif defined(CAMERONS_HAWAIIAN_BLEND)
const float TEMP_SETPOINTS [4] = {140, 100, 95, 95};
#else // No-heat
const float TEMP_SETPOINTS [4] = {0,0,0,0};
#endif

const float PID_GAIN_P = 0.05;
const float PID_GAIN_I = 0.0015;
const float PID_GAIN_D = 0.0005;
const float PID_GAIN_F = 0.00005;

const float SCALE_CONVERSION_MG = -0.152710615479;

const unsigned int BREW_DOSE_MG = 16000;
const unsigned int BREW_YIELD_MG = 30000;
const unsigned int AUTOBREW_PREINFUSE_END_POWER    = 80;
const unsigned int AUTOBREW_PREINFUSE_ON_TIME_US   = 4000000;
const unsigned int AUTOBREW_PREINFUSE_OFF_TIME_US  = 4000000;
const unsigned int AUTOBREW_BREW_RAMP_TIME         = 1000000;   
const unsigned int AUTOBREW_BREW_TIMEOUT_US        = 60000000; // 60s

/** \brief The time parameters associated with the espresso machine */
typedef struct {
    uint8_t pre_on_ds;  /**< The duration of the preinfuse on time in s/10 */
    uint8_t pre_off_ds; /**< The duration of the preinfuse off time in s/10 */
    uint8_t timeout_s;  /**< The maximum duration of the autobrew routine in s */
    uint8_t ramp_ds;    /**< The duration of the power ramp in s/10 */
} brew_times;

/** \brief The weight parameters associated with the espresso machine */
typedef struct {
    uint16_t dose_cg;  /**< The weight of the grounds used in g/100 */
    uint16_t yield_cg; /**< The weight of espresso output in g/100 */
} brew_weights;

typedef struct {
    uint16_t brew_dc;
    uint16_t hot_dc;
    uint16_t steam_dc;
} brew_temps;

typedef struct {
    uint8_t pre_per;
    uint8_t brew_per;
} brew_powers;

typedef struct {
    brew_times   time;
    brew_weights weight;
    brew_temps   temp;
    brew_powers  power;
} machine_settings;

/** \brief Initalize the settings and attach to memory device. 
 * \param mem A pointer to an initalized mb85_fram structure.
 * \returns A const pointer to the global settings structure or NULL on error. 
*/
const machine_settings * machine_settings_setup(mb85_fram * mem);

/** \brief Get const pointer to internal settings structure. 
 * \returns Pointer to internal settings structure or NULL if not setup.
*/
const machine_settings * machine_settings_aquire();

/**
 * \brief Save the current internal settings structure as the indicated ID.
 * 
 * \param profile A value from 0 to 8 indicating the profile to save to.
 * 
 * \returns PICO_ERROR_NONE if successfull. Else error code. 
*/
int machine_settings_save_profile(uint8_t * profile);

/**
 * \brief Load the idicated profile int the internal settings structure.
 * 
 * If any value in the profile is unsafe, they will all be set to default values.
 * 
 * \param profile A value from 0 to 8 indicating the profile to load.
 * 
 * \returns PICO_ERROR_NONE if successfull. Else error code. 
*/
int machine_settings_load_profile(uint8_t * profile);

int machine_settings_set_time_pre_on(uint8_t t_ds);
int machine_settings_set_time_pre_off(uint8_t t_ds);
int machine_settings_set_time_timeout(uint8_t t_s);
int machine_settings_set_time_ramp(uint8_t t_ds);

int machine_settings_set_weight_dose(uint8_t cg);
int machine_settings_set_weight_yield(uint8_t cg);

int machine_settings_set_temp_brew(uint16_t t_dc);
int machine_settings_set_temp_hot(uint16_t t_dc);
int machine_settings_set_temp_steam(uint16_t t_dc);

int machine_settings_set_power_pre(uint8_t pwr);
int machine_settings_set_power_brew(uint8_t pwr);
#endif