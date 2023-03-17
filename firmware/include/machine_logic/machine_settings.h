/** \defgroup machine_settings Machine Settings Library
 *  \ingroup machine_logic
 * 
 *  \brief Library managing the settings of a single boiler espresso machine.
 * 
 *  The machine_settings functionality can be divided into three groups. First, the settings
 *  must be accessible by external programs. This is accomplished with const machine_settings 
 *  pointers that are passed out after calling ::machine_settings_setup or ::machine_settings_acquire.
 *  This allows for access to the current settings with, for example, 
 *  
 *      ms = machine_settings_acquire();
 *      weight_dg yield = *(ms->brew.yield);
 *  
 *  Note the field names are pointers themselves and must be dereferenced. 
 *  
 *  The library also handles the modification of settings. This is accomplished with the
 *  ::machine_settings_update function. This function takes a reset and select flags and a integer 
 *  value. The reset key is like pressing the escape key and the select is like pressing return. 
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
 * \version 0.1
 * \date 2022-11-12
 */

#ifndef MACHINE_SETTINGS_H
#define MACHINE_SETTINGS_H

#include "pico/stdlib.h" // Typedefs
#include "drivers/mb85_fram.h"   // FRAM memory driver to store settings

typedef int16_t machine_setting;     /**< \brief Generic machine setting field */
typedef machine_setting temp_dC;     /**< \brief Temperature value. Divide by 10 to get C */
typedef machine_setting duration_ds; /**< \brief Duration length. Divide by 10 to get seconds */
typedef machine_setting duration_s;  /**< \brief Duration length in seconds */
typedef machine_setting power_per;   /**< \brief Power level as percentage */
typedef machine_setting flow_ul_ds;  /**< \brief Flow in microliters per second */
typedef machine_setting weight_dg;   /**< \brief Weight value. Divide by 10 to get g */

/** \brief The settings associated with the steam mode */
typedef struct {
    temp_dC * temp; /**< The target temperature when in steam mode */
} machine_settings_steam;

/** \brief The settings associated with the hot-water mode */
typedef struct {
    temp_dC *   temp;  /**< The target temperature when in hot-water mode */
    power_per * power; /**< The pump power for when in steam mode */
} machine_settings_hot;

/** \brief The settings associated with brewing espresso. 
 * 
 * Used in manual and auto mode 
 * */
typedef struct {
    temp_dC *   temp;  /**< The target temperature when brewing */ 
    power_per * power; /**< The pump power when brewing */ 
    weight_dg * dose;  /**< Weight of grounds used when brewing */
    weight_dg * yield; /**< Weight of espresso to brew */
} machine_settings_brew;

/** \brief The settings associated with the auto mode */
typedef struct {
    duration_s *  preinf_timeout;   /**< Length of time to soak puck during pre-infuse */
    power_per *   preinf_power;     /**< The power of the pre-infuse routine */
    duration_ds * preinf_ramp_time; /**< The time, in ds of the linear ramp to target power */
    flow_ul_ds *  flow;             /**< The target flow rate during the main brew leg */
    duration_s *  timeout;          /**< The length of time to attempt to reach yield */
} machine_settings_auto;

/** \brief Full espresso machine settings. 
 * 
 * The fields are associated with the different modes and the ui_mask uses the lowest three 
 * bits to communicate the current value of a setting being modifed. 
*/
typedef struct {
    machine_settings_auto  autobrew;  /**< The settings associated with the auto mode */
    machine_settings_brew  brew;      /**< The settings associated with brewing espresso. Used in manual and auto mode */
    machine_settings_hot   hot;       /**< The settings associated with the hot-water mode */
    machine_settings_steam steam;     /**< The settings associated with the steam mode */
    uint8_t ui_mask;                  /**< Bitfield whose lowest 3 bits communicate the state of setting modifications */
} machine_settings;

/** \brief Initialize the settings and attach to memory device. 
 * \param mem A pointer to an initalized mb85_fram structure.
 * \returns A const pointer to the global settings structure or NULL on error. 
*/
const machine_settings * machine_settings_setup(mb85_fram * mem);

/** \brief Get a const pointer to internal settings structure
 * \returns A const pointer to the global settings structure or NULL if not setup. 
*/
machine_settings * machine_settings_acquire();

/**
 * \brief Navigates the internal setting's tree and updates values accordingly
 * 
 * \param reset Flag indicating if the internal UI should be reset.
 * \param select Flag indicating if select condition is met
 * \param val Value of selector
 * \return int 
 */
int machine_settings_update(bool reset, bool select, uint8_t val);

/**
 * \brief Print the current states using printf
 * 
 * \return PICO_ERROR_NONE if setup. Else PICO_ERROR_GENERIC.
 */
int machine_settings_print();
#endif
/** @} */