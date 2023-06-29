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
 * \version 0.1
 * \date 2022-11-12
 */

#ifndef MACHINE_SETTINGS_H
#define MACHINE_SETTINGS_H

#include "pico/stdlib.h" // Typedefs
#include "drivers/mb85_fram.h"   // FRAM memory driver to store settings

#define NUM_AUTOBREW_LEGS 9
#define NUM_AUTOBREW_PARAMS_PER_LEG 7

typedef int16_t machine_setting; /**< \brief Generic machine setting field with range from -32768 to 32767 */

/** \brief Enumerated list naming the indicies of the settings array. */
typedef enum {MS_TEMP_BREW = 0,
              MS_TEMP_HOT,
              MS_TEMP_STEAM,
              MS_WEIGHT_DOSE,
              MS_WEIGHT_YIELD,
              MS_POWER_BREW,     
              MS_POWER_HOT,
              MS_A1_REF_STYLE,
              MS_A1_REF_START,
              MS_A1_REF_END,
              MS_A1_TRGR_FLOW,
              MS_A1_TRGR_PRSR,
              MS_A1_TRGR_MASS,
              MS_A1_TIMEOUT,
              MS_A2_REF_STYLE,
              MS_A2_REF_START,
              MS_A2_REF_END,
              MS_A2_TRGR_FLOW,
              MS_A2_TRGR_PRSR,
              MS_A2_TRGR_MASS,
              MS_A2_TIMEOUT,
              MS_A3_REF_STYLE,
              MS_A3_REF_START,
              MS_A3_REF_END,
              MS_A3_TRGR_FLOW,
              MS_A3_TRGR_PRSR,
              MS_A3_TRGR_MASS,
              MS_A3_TIMEOUT,
              MS_A4_REF_STYLE,
              MS_A4_REF_START,
              MS_A4_REF_END,
              MS_A4_TRGR_FLOW,
              MS_A4_TRGR_PRSR,
              MS_A4_TRGR_MASS,
              MS_A4_TIMEOUT,
              MS_A5_REF_STYLE,
              MS_A5_REF_START,
              MS_A5_REF_END,
              MS_A5_TRGR_FLOW,
              MS_A5_TRGR_PRSR,
              MS_A5_TRGR_MASS,
              MS_A5_TIMEOUT,
              MS_A6_REF_STYLE,
              MS_A6_REF_START,
              MS_A6_REF_END,
              MS_A6_TRGR_FLOW,
              MS_A6_TRGR_PRSR,
              MS_A6_TRGR_MASS,
              MS_A6_TIMEOUT,
              MS_A7_REF_STYLE,
              MS_A7_REF_START,
              MS_A7_REF_END,
              MS_A7_TRGR_FLOW,
              MS_A7_TRGR_PRSR,
              MS_A7_TRGR_MASS,
              MS_A7_TIMEOUT,
              MS_A8_REF_STYLE,
              MS_A8_REF_START,
              MS_A8_REF_END,
              MS_A8_TRGR_FLOW,
              MS_A8_TRGR_PRSR,
              MS_A8_TRGR_MASS,
              MS_A8_TIMEOUT,
              MS_A9_REF_STYLE,
              MS_A9_REF_START,
              MS_A9_REF_END,
              MS_A9_TRGR_FLOW,
              MS_A9_TRGR_PRSR,
              MS_A9_TRGR_MASS,
              MS_A9_TIMEOUT,
              NUM_SETTINGS,
              MS_UI_MASK} setting_id;

enum {AUTOBREW_REF_STYLE_PWR = 0, AUTOBREW_REF_STYLE_FLOW, AUTOBREW_REF_STYLE_PRSR};

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
int machine_settings_update(bool reset, bool select, uint8_t val);

/**
 * \brief Print the current states using printf
 * 
 * \return PICO_ERROR_NONE if setup. Else PICO_ERROR_GENERIC.
 */
int machine_settings_print();
#endif
/** @} */