/** \defgroup machine_settings Machine Settings Library
 *  \ingroup machine_logic
 * 
 *  \brief Library managing the settings of a single boiler espresso machine.
 * 
 *  The machine_settings functionality can be divided into three groups. First, the settings
 *  must be accessible by external programs. This is accomplished with the ::machine_settings_get
 *  function which takes one, enumerated parameter indicating which setting to retrieve.
 *  
 *  The library also handles the modification of settings. This is accomplished with the
 *  ::machine_settings_update function. This function takes a reset and select flags and a integer 
 *  value. The reset key is like pressing the escape key, and the select is like pressing return. 
 *  Finally, the integer value is like highlighting one of four values to select. An example 
 *  sequence that would adjust the brew temp by +2C is (Settings + RET) -> (Temp + RET) -> 
 *  (Brew + RET) -> (+1C + RET) -> (+1C + RET) -> (ESC). When adjusting any value, it is displayed 
 *  using a value flasher attached to the ui_mask field of the internal settings structure.
 *  
 *  Finally, the library also saves and loads the machine settings. This is handled much like
 *  the value modification using the machine_settings_update function. An example to load from
 *  profile 5 would be (Profiles + RET) -> (Profiles 4-6 + RET) -> (Profiles 5 + RET) -> (Load +
 *  RET) -> (ESC). Any changes made to the profile must be manually saved with a similar process.
 *  However, the current state of the settings is maintained between startups. 
 *  @{
 * 
 * \file machine_settings.h
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Machine Settings header
 * \version 1.0
 * \date 2023-07-01
 */

#ifndef MACHINE_SETTINGS_H
#define MACHINE_SETTINGS_H

#include "pico/stdlib.h"         // Typedefs
#include "drivers/mb85_fram.h"   // FRAM memory driver to store settings

#define NUM_AUTOBREW_LEGS 9           /**<\brief The max number of autobrew legs in the settings. */
#define NUM_AUTOBREW_PARAMS_PER_LEG 7 /**<\brief The number of settings per autobrew leg. */

typedef int32_t machine_setting; /**< \brief Generic machine setting field with range from 0 to 65535 */

/** \brief Enumerated list naming the indicies of the settings array. */
typedef enum {
    MS_TEMP_BREW_cC = 0,              /**<\brief The temp for brew and autobrew mode. */
    MS_TEMP_HOT_cC,                   /**<\brief The temp for hot-water mode. */
    MS_TEMP_STEAM_cC,                 /**<\brief The temp for steam mode. */
    MS_WEIGHT_DOSE_mg,                /**<\brief The target dose. */
    MS_WEIGHT_YIELD_mg,               /**<\brief The target yield. */
    MS_POWER_BREW_PER,                /**<\brief Pump power while in brew mode. */
    MS_POWER_HOT_PER,                 /**<\brief Pump power while in hot-water mode. */
    MS_A1_REF_STYLE_ENM,              /**<\brief The reference style (PWR, FLOW, or PRSR) for autobrew leg 1. */
    MS_A1_REF_START_100per_ulps_mbar, /**<\brief The starting reference for autobrew leg 1. If percent, divide by 100. Else, use directly. */
    MS_A1_REF_END_100per_ulps_mbar,   /**<\brief The ending reference for autobrew leg 1. If percent, divide by 100. Else, use directly. */
    MS_A1_TRGR_FLOW_ul_s,             /**<\brief Flow that triggers autobrew leg 1 to end. Set to 0 to disable. */
    MS_A1_TRGR_PRSR_mbar,             /**<\brief Pressure that triggers autobrew leg 1 to end. Set to 0 to disable. */
    MS_A1_TRGR_MASS_mg,               /**<\brief Weight that triggers autobrew leg 1 to end. Set to 0 to disable. */
    MS_A1_TIMEOUT_ms,                 /**<\brief Time that triggers autobrew leg 1 to end. Set to 0 to disable leg. */
    MS_A2_REF_STYLE_ENM,              /**<\brief The reference style (PWR, FLOW, or PRSR) for autobrew leg 2. */
    MS_A2_REF_START_100per_ulps_mbar, /**<\brief The starting reference for autobrew leg 2. If percent, divide by 100. Else, use directly. */
    MS_A2_REF_END_100per_ulps_mbar,   /**<\brief The ending reference for autobrew leg 2. If percent, divide by 100. Else, use directly. */
    MS_A2_TRGR_FLOW_ul_s,             /**<\brief Flow that triggers autobrew leg 2 to end. Set to 0 to disable. */
    MS_A2_TRGR_PRSR_mbar,             /**<\brief Pressure that triggers autobrew leg 2 to end. Set to 0 to disable. */
    MS_A2_TRGR_MASS_mg,               /**<\brief Weight that triggers autobrew leg 2 to end. Set to 0 to disable. */
    MS_A2_TIMEOUT_ms,                 /**<\brief Time that triggers autobrew leg 2 to end. Set to 0 to disable leg. */
    MS_A3_REF_STYLE_ENM,              /**<\brief The reference style (PWR, FLOW, or PRSR) for autobrew leg 3. */
    MS_A3_REF_START_100per_ulps_mbar, /**<\brief The starting reference for autobrew leg 3. If percent, divide by 100. Else, use directly. */
    MS_A3_REF_END_100per_ulps_mbar,   /**<\brief The ending reference for autobrew leg 3. If percent, divide by 100. Else, use directly. */
    MS_A3_TRGR_FLOW_ul_s,             /**<\brief Flow that triggers autobrew leg 3 to end. Set to 0 to disable. */
    MS_A3_TRGR_PRSR_mbar,             /**<\brief Pressure that triggers autobrew leg 3 to end. Set to 0 to disable. */
    MS_A3_TRGR_MASS_mg,               /**<\brief Weight that triggers autobrew leg 3 to end. Set to 0 to disable. */
    MS_A3_TIMEOUT_ms,                 /**<\brief Time that triggers autobrew leg 3 to end. Set to 0 to disable leg. */
    MS_A4_REF_STYLE_ENM,              /**<\brief The reference style (PWR, FLOW, or PRSR) for autobrew leg 4. */
    MS_A4_REF_START_100per_ulps_mbar, /**<\brief The starting reference for autobrew leg 4. If percent, divide by 100. Else, use directly. */
    MS_A4_REF_END_100per_ulps_mbar,   /**<\brief The ending reference for autobrew leg 4. If percent, divide by 100. Else, use directly. */
    MS_A4_TRGR_FLOW_ul_s,             /**<\brief Flow that triggers autobrew leg 4 to end. Set to 0 to disable. */
    MS_A4_TRGR_PRSR_mbar,             /**<\brief Pressure that triggers autobrew leg 4 to end. Set to 0 to disable. */
    MS_A4_TRGR_MASS_mg,               /**<\brief Weight that triggers autobrew leg 4 to end. Set to 0 to disable. */
    MS_A4_TIMEOUT_ms,                 /**<\brief Time that triggers autobrew leg 4 to end. Set to 0 to disable leg. */
    MS_A5_REF_STYLE_ENM,              /**<\brief The reference style (PWR, FLOW, or PRSR) for autobrew leg 5. */
    MS_A5_REF_START_100per_ulps_mbar, /**<\brief The starting reference for autobrew leg 5. If percent, divide by 100. Else, use directly. */
    MS_A5_REF_END_100per_ulps_mbar,   /**<\brief The ending reference for autobrew leg 5. If percent, divide by 100. Else, use directly. */
    MS_A5_TRGR_FLOW_ul_s,             /**<\brief Flow that triggers autobrew leg 5 to end. Set to 0 to disable. */
    MS_A5_TRGR_PRSR_mbar,             /**<\brief Pressure that triggers autobrew leg 5 to end. Set to 0 to disable. */
    MS_A5_TRGR_MASS_mg,               /**<\brief Weight that triggers autobrew leg 5 to end. Set to 0 to disable. */
    MS_A5_TIMEOUT_ms,                 /**<\brief Time that triggers autobrew leg 5 to end. Set to 0 to disable leg. */
    MS_A6_REF_STYLE_ENM,              /**<\brief The reference style (PWR, FLOW, or PRSR) for autobrew leg 6. */
    MS_A6_REF_START_100per_ulps_mbar, /**<\brief The starting reference for autobrew leg 6. If percent, divide by 100. Else, use directly. */
    MS_A6_REF_END_100per_ulps_mbar,   /**<\brief The ending reference for autobrew leg 6. If percent, divide by 100. Else, use directly. */
    MS_A6_TRGR_FLOW_ul_s,             /**<\brief Flow that triggers autobrew leg 6 to end. Set to 0 to disable. */
    MS_A6_TRGR_PRSR_mbar,             /**<\brief Pressure that triggers autobrew leg 6 to end. Set to 0 to disable. */
    MS_A6_TRGR_MASS_mg,               /**<\brief Weight that triggers autobrew leg 6 to end. Set to 0 to disable. */
    MS_A6_TIMEOUT_ms,                 /**<\brief Time that triggers autobrew leg 6 to end. Set to 0 to disable leg. */
    MS_A7_REF_STYLE_ENM,              /**<\brief The reference style (PWR, FLOW, or PRSR) for autobrew leg 7. */
    MS_A7_REF_START_100per_ulps_mbar, /**<\brief The starting reference for autobrew leg 7. If percent, divide by 100. Else, use directly. */
    MS_A7_REF_END_100per_ulps_mbar,   /**<\brief The ending reference for autobrew leg 7. If percent, divide by 100. Else, use directly. */
    MS_A7_TRGR_FLOW_ul_s,             /**<\brief Flow that triggers autobrew leg 7 to end. Set to 0 to disable. */
    MS_A7_TRGR_PRSR_mbar,             /**<\brief Pressure that triggers autobrew leg 7 to end. Set to 0 to disable. */
    MS_A7_TRGR_MASS_mg,               /**<\brief Weight that triggers autobrew leg 7 to end. Set to 0 to disable. */
    MS_A7_TIMEOUT_ms,                 /**<\brief Time that triggers autobrew leg 7 to end. Set to 0 to disable leg. */
    MS_A8_REF_STYLE_ENM,              /**<\brief The reference style (PWR, FLOW, or PRSR) for autobrew leg 8. */
    MS_A8_REF_START_100per_ulps_mbar, /**<\brief The starting reference for autobrew leg 8. If percent, divide by 100. Else, use directly. */
    MS_A8_REF_END_100per_ulps_mbar,   /**<\brief The ending reference for autobrew leg 8. If percent, divide by 100. Else, use directly. */
    MS_A8_TRGR_FLOW_ul_s,             /**<\brief Flow that triggers autobrew leg 8 to end. Set to 0 to disable. */
    MS_A8_TRGR_PRSR_mbar,             /**<\brief Pressure that triggers autobrew leg 8 to end. Set to 0 to disable. */
    MS_A8_TRGR_MASS_mg,               /**<\brief Weight that triggers autobrew leg 8 to end. Set to 0 to disable. */
    MS_A8_TIMEOUT_ms,                 /**<\brief Time that triggers autobrew leg 8 to end. Set to 0 to disable leg. */
    MS_A9_REF_STYLE_ENM,              /**<\brief The reference style (PWR, FLOW, or PRSR) for autobrew leg 9. */
    MS_A9_REF_START_100per_ulps_mbar, /**<\brief The starting reference for autobrew leg 9. If percent, divide by 100. Else, use directly. */
    MS_A9_REF_END_100per_ulps_mbar,   /**<\brief The ending reference for autobrew leg 9. If percent, divide by 100. Else, use directly. */
    MS_A9_TRGR_FLOW_ul_s,             /**<\brief Flow that triggers autobrew leg 9 to end. Set to 0 to disable. */
    MS_A9_TRGR_PRSR_mbar,             /**<\brief Pressure that triggers autobrew leg 9 to end. Set to 0 to disable. */
    MS_A9_TRGR_MASS_mg,               /**<\brief Weight that triggers autobrew leg 9 to end. Set to 0 to disable. */
    MS_A9_TIMEOUT_ms,                 /**<\brief Time that triggers autobrew leg 9 to end. Set to 0 to disable leg. */
    NUM_SETTINGS,                     /**<\brief The number of settings that are managed. */
    MS_UI_MASK                        /**<\brief ui mask for flashing values on LEDs */
} setting_id;

/**\brief Enumerated list of possible reference styles. */
enum {
    AUTOBREW_REF_STYLE_PWR = 0, /**<\brief The reference is percent power. */
    AUTOBREW_REF_STYLE_FLOW,    /**<\brief The reference is flowrate. */
    AUTOBREW_REF_STYLE_PRSR     /**<\brief The reference is pressure. */
    };

typedef enum {
    MS_CMD_NONE = '0',
    MS_CMD_SUBFOLDER_A = '1',
    MS_CMD_SUBFOLDER_B = '2',
    MS_CMD_SUBFOLDER_C = '3',
    MS_CMD_ROOT = 'r',
    MS_CMD_UP = 'u',
    MS_CMD_PRINT = 'p'
} setting_command;

/** \brief Initialize the settings and attach to memory device. 
 * \param mem A pointer to an initalized mb85_fram structure.
*/
void machine_settings_setup(mb85_fram mem);

/** 
 * \brief Get the current value of a setting or the UI mask
 * \returns The value of the indicated setting or the UI mask
*/
machine_setting machine_settings_get(setting_id id);

/**
 * \brief Navigates the internal setting's tree and updates values accordingly
 * 
 * \param reset Flag indicating if the internal UI should be reset.
 * \param select Flag indicating if select condition is met
 * \param val Value of selector
 * \return int 
 */
int machine_settings_update(setting_command cmd);

/**
 * \brief Print the current states using printf
 * 
 * \return PICO_ERROR_NONE if setup. Else PICO_ERROR_GENERIC.
 */
int machine_settings_print();
#endif
/** @} */