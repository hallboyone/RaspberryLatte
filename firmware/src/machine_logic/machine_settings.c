/**
 * @ingroup machine_settings
 * @{
 * 
 * \file machine_settings.c
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Machine Settings source
 * \version 0.1
 * \date 2022-11-12
 */

#include "machine_logic/machine_settings.h"

#include <stdio.h>
#include <string.h>

#include "machine_logic/local_ui.h"
#include "utils/value_flasher.h"
#include "utils/macros.h"

/** \brief Starting address in mb85 FRAM chip where setting data is stored */
static const reg_addr MACHINE_SETTINGS_START_ADDR = 0x0000;

/** \brief The size, in bytes, of a single settings profile */
static const uint16_t MACHINE_SETTINGS_MEMORY_SIZE = NUM_SETTINGS * sizeof(machine_setting);

/** \brief Pointer to FRAM memory IC object where settings are stored */
static mb85_fram _mem = NULL;

/** \brief Flasher object to display a setting when it's getting modified. */
static value_flasher _setting_flasher;
static uint8_t _ui_mask;

/** \brief Internal settings array holding the master settings */
static machine_setting _ms [NUM_SETTINGS];
typedef struct{
    int8_t step_size;    /**\brief Length of each delta when incremented. 0 is for enumerated values. */
    machine_setting max; /**\brief The maximum value of the setting. */
    machine_setting std; /**\brief The default value of the setting. */
} machine_setting_specs;

/** \brief Minimum settings array */
static const machine_setting_specs _specs [NUM_SETTINGS] = {
    {.step_size = 100, .max = 14000, .std = 9000 },// MS_TEMP_BREW_cC = 0
    {.step_size = 100, .max = 14000, .std = 10000},// MS_TEMP_HOT_cC
    {.step_size = 100, .max = 14000, .std = 14000},// MS_TEMP_STEAM_cC
    {.step_size = 100, .max = 30000, .std = 15000 },// MS_WEIGHT_DOSE_mg
    {.step_size = 100, .max = 60000, .std = 30000 },// MS_WEIGHT_YIELD_mg
    {.step_size = 1,   .max = 100,   .std = 100   },// MS_POWER_BREW_PER,   
    {.step_size = 1,   .max = 100,   .std = 20    },// MS_POWER_HOT_PER
    {.step_size = 0,   .max = 3,     .std = 0     },// MS_A1_REF_STYLE_ENM
    {.step_size = 100, .max = 20000, .std = 2500  },// MS_A1_REF_START_100per_ulps_mbar
    {.step_size = 100, .max = 20000, .std = 2500  },// MS_A1_REF_END_100per_ulps_mbar
    {.step_size = 100, .max = 20000, .std = 0     },// MS_A1_TRGR_FLOW_ul_s
    {.step_size = 100, .max = 15000, .std = 0     },// MS_A1_TRGR_PRSR_mbar
    {.step_size = 100, .max = 30000, .std = 0     },// MS_A1_TRGR_MASS_mg
    {.step_size = 1,   .max = 60,    .std = 0     },// MS_A1_TIMEOUT_s
    {.step_size = 0,   .max = 3,     .std = 0     },// MS_A2_REF_STYLE_ENM
    {.step_size = 100, .max = 20000, .std = 2500  },// MS_A2_REF_START_100per_ulps_mbar
    {.step_size = 100, .max = 20000, .std = 2500  },// MS_A2_REF_END_100per_ulps_mbar
    {.step_size = 100, .max = 20000, .std = 0     },// MS_A2_TRGR_FLOW_ul_s
    {.step_size = 100, .max = 15000, .std = 0     },// MS_A2_TRGR_PRSR_mbar
    {.step_size = 100, .max = 30000, .std = 0     },// MS_A2_TRGR_MASS_mg
    {.step_size = 1,   .max = 60,    .std = 0     },// MS_A2_TIMEOUT_s
    {.step_size = 0,   .max = 3,     .std = 0     },// MS_A3_REF_STYLE_ENM
    {.step_size = 100, .max = 20000, .std = 2500  },// MS_A3_REF_START_100per_ulps_mbar
    {.step_size = 100, .max = 20000, .std = 2500  },// MS_A3_REF_END_100per_ulps_mbar
    {.step_size = 100, .max = 20000, .std = 0     },// MS_A3_TRGR_FLOW_ul_s
    {.step_size = 100, .max = 15000, .std = 0     },// MS_A3_TRGR_PRSR_mbar
    {.step_size = 100, .max = 30000, .std = 0     },// MS_A3_TRGR_MASS_mg
    {.step_size = 1,   .max = 60,    .std = 0     },// MS_A3_TIMEOUT_s
    {.step_size = 0,   .max = 3,     .std = 0     },// MS_A4_REF_STYLE_ENM
    {.step_size = 100, .max = 20000, .std = 2500  },// MS_A4_REF_START_100per_ulps_mbar
    {.step_size = 100, .max = 20000, .std = 2500  },// MS_A4_REF_END_100per_ulps_mbar
    {.step_size = 100, .max = 20000, .std = 0     },// MS_A4_TRGR_FLOW_ul_s
    {.step_size = 100, .max = 15000, .std = 0     },// MS_A4_TRGR_PRSR_mbar
    {.step_size = 100, .max = 30000, .std = 0     },// MS_A4_TRGR_MASS_mg
    {.step_size = 1,   .max = 60,    .std = 0     },// MS_A4_TIMEOUT_s
    {.step_size = 0,   .max = 3,     .std = 0     },// MS_A5_REF_STYLE_ENM
    {.step_size = 100, .max = 20000, .std = 2500  },// MS_A5_REF_START_100per_ulps_mbar
    {.step_size = 100, .max = 20000, .std = 2500  },// MS_A5_REF_END_100per_ulps_mbar
    {.step_size = 100, .max = 20000, .std = 0     },// MS_A5_TRGR_FLOW_ul_s
    {.step_size = 100, .max = 15000, .std = 0     },// MS_A5_TRGR_PRSR_mbar
    {.step_size = 100, .max = 30000, .std = 0     },// MS_A5_TRGR_MASS_mg
    {.step_size = 1,   .max = 60,    .std = 0     },// MS_A5_TIMEOUT_s
    {.step_size = 0,   .max = 3,     .std = 0     },// MS_A6_REF_STYLE_ENM
    {.step_size = 100, .max = 20000, .std = 2500  },// MS_A6_REF_START_100per_ulps_mbar
    {.step_size = 100, .max = 20000, .std = 2500  },// MS_A6_REF_END_100per_ulps_mbar
    {.step_size = 100, .max = 20000, .std = 0     },// MS_A6_TRGR_FLOW_ul_s
    {.step_size = 100, .max = 15000, .std = 0     },// MS_A6_TRGR_PRSR_mbar
    {.step_size = 100, .max = 30000, .std = 0     },// MS_A6_TRGR_MASS_mg
    {.step_size = 1,   .max = 60,    .std = 0     },// MS_A6_TIMEOUT_s
    {.step_size = 0,   .max = 3,     .std = 0     },// MS_A7_REF_STYLE_ENM
    {.step_size = 100, .max = 20000, .std = 2500  },// MS_A7_REF_START_100per_ulps_mbar
    {.step_size = 100, .max = 20000, .std = 2500  },// MS_A7_REF_END_100per_ulps_mbar
    {.step_size = 100, .max = 20000, .std = 0     },// MS_A7_TRGR_FLOW_ul_s
    {.step_size = 100, .max = 15000, .std = 0     },// MS_A7_TRGR_PRSR_mbar
    {.step_size = 100, .max = 30000, .std = 0     },// MS_A7_TRGR_MASS_mg
    {.step_size = 1,   .max = 60,    .std = 0     },// MS_A7_TIMEOUT_s
    {.step_size = 0,   .max = 3,     .std = 0     },// MS_A8_REF_STYLE_ENM
    {.step_size = 100, .max = 20000, .std = 2500  },// MS_A8_REF_START_100per_ulps_mbar
    {.step_size = 100, .max = 20000, .std = 2500  },// MS_A8_REF_END_100per_ulps_mbar
    {.step_size = 100, .max = 20000, .std = 0     },// MS_A8_TRGR_FLOW_ul_s
    {.step_size = 100, .max = 15000, .std = 0     },// MS_A8_TRGR_PRSR_mbar
    {.step_size = 100, .max = 30000, .std = 0     },// MS_A8_TRGR_MASS_mg
    {.step_size = 1,   .max = 60,    .std = 0     },// MS_A8_TIMEOUT_s
    {.step_size = 0,   .max = 3,     .std = 0     },// MS_A9_REF_STYLE_ENM
    {.step_size = 100, .max = 20000, .std = 2500  },// MS_A9_REF_START_100per_ulps_mbar
    {.step_size = 100, .max = 20000, .std = 2500  },// MS_A9_REF_END_100per_ulps_mbar
    {.step_size = 100, .max = 20000, .std = 0     },// MS_A9_TRGR_FLOW_ul_s
    {.step_size = 100, .max = 15000, .std = 0     },// MS_A9_TRGR_PRSR_mbar
    {.step_size = 100, .max = 30000, .std = 0     },// MS_A9_TRGR_MASS_mg
    {.step_size = 1,   .max = 60,    .std = 0     } // MS_A9_TIMEOUT_s
    };

typedef local_ui_folder luf;
static local_ui_folder_tree settings_modifier; /**< \brief Local UI folder tree for updating machine settings*/
static luf  f_;
static luf      f_set;
static luf          f_set_temp;
static luf              f_set_temp_brew;
static luf              f_set_temp_hot;
static luf              f_set_temp_steam;
static luf          f_set_weight;
static luf              f_set_weight_yield;
static luf              f_set_weight_dose; 
static luf          f_set_power;
static luf              f_set_power_brew;
static luf              f_set_power_hot;
static luf      f_ab;
static luf          f_ab_ab13;
static luf              f_ab_ab13_ab1;
static luf                  f_ab_ab13_ab1_ref;
static luf                      f_ab_ab13_ab1_ref_style;
static luf                      f_ab_ab13_ab1_ref_start;
static luf                      f_ab_ab13_ab1_ref_end;
static luf                  f_ab_ab13_ab1_trgr;
static luf                      f_ab_ab13_ab1_trgr_flow;
static luf                      f_ab_ab13_ab1_trgr_prsr;
static luf                      f_ab_ab13_ab1_trgr_mass;
static luf                  f_ab_ab13_ab1_timeout;
static luf              f_ab_ab13_ab2;
static luf                  f_ab_ab13_ab2_ref;
static luf                      f_ab_ab13_ab2_ref_style;
static luf                      f_ab_ab13_ab2_ref_start;
static luf                      f_ab_ab13_ab2_ref_end;
static luf                  f_ab_ab13_ab2_trgr;
static luf                      f_ab_ab13_ab2_trgr_flow;
static luf                      f_ab_ab13_ab2_trgr_prsr;
static luf                      f_ab_ab13_ab2_trgr_mass;
static luf                  f_ab_ab13_ab2_timeout;
static luf              f_ab_ab13_ab3;
static luf                  f_ab_ab13_ab3_ref;
static luf                      f_ab_ab13_ab3_ref_style;
static luf                      f_ab_ab13_ab3_ref_start;
static luf                      f_ab_ab13_ab3_ref_end;
static luf                  f_ab_ab13_ab3_trgr;
static luf                      f_ab_ab13_ab3_trgr_flow;
static luf                      f_ab_ab13_ab3_trgr_prsr;
static luf                      f_ab_ab13_ab3_trgr_mass;
static luf                  f_ab_ab13_ab3_timeout;
static luf          f_ab_ab46;
static luf              f_ab_ab46_ab4;
static luf                  f_ab_ab46_ab4_ref;
static luf                      f_ab_ab46_ab4_ref_style;
static luf                      f_ab_ab46_ab4_ref_start;
static luf                      f_ab_ab46_ab4_ref_end;
static luf                  f_ab_ab46_ab4_trgr;
static luf                      f_ab_ab46_ab4_trgr_flow;
static luf                      f_ab_ab46_ab4_trgr_prsr;
static luf                      f_ab_ab46_ab4_trgr_mass;
static luf                  f_ab_ab46_ab4_timeout;
static luf              f_ab_ab46_ab5;
static luf                  f_ab_ab46_ab5_ref;
static luf                      f_ab_ab46_ab5_ref_style;
static luf                      f_ab_ab46_ab5_ref_start;
static luf                      f_ab_ab46_ab5_ref_end; 
static luf                  f_ab_ab46_ab5_trgr;
static luf                      f_ab_ab46_ab5_trgr_flow;
static luf                      f_ab_ab46_ab5_trgr_prsr;
static luf                      f_ab_ab46_ab5_trgr_mass;
static luf                  f_ab_ab46_ab5_timeout;
static luf              f_ab_ab46_ab6;
static luf                  f_ab_ab46_ab6_ref;
static luf                      f_ab_ab46_ab6_ref_style;
static luf                      f_ab_ab46_ab6_ref_start;
static luf                      f_ab_ab46_ab6_ref_end;
static luf                  f_ab_ab46_ab6_trgr;
static luf                      f_ab_ab46_ab6_trgr_flow;
static luf                      f_ab_ab46_ab6_trgr_prsr;
static luf                      f_ab_ab46_ab6_trgr_mass;
static luf                  f_ab_ab46_ab6_timeout;
static luf          f_ab_ab79;
static luf              f_ab_ab79_ab7;
static luf                  f_ab_ab79_ab7_ref;
static luf                      f_ab_ab79_ab7_ref_style;
static luf                      f_ab_ab79_ab7_ref_start;
static luf                      f_ab_ab79_ab7_ref_end;
static luf                  f_ab_ab79_ab7_trgr;
static luf                      f_ab_ab79_ab7_trgr_flow;
static luf                      f_ab_ab79_ab7_trgr_prsr;
static luf                      f_ab_ab79_ab7_trgr_mass;
static luf                  f_ab_ab79_ab7_timeout;
static luf              f_ab_ab79_ab8;
static luf                  f_ab_ab79_ab8_ref;
static luf                      f_ab_ab79_ab8_ref_style;
static luf                      f_ab_ab79_ab8_ref_start;
static luf                      f_ab_ab79_ab8_ref_end; 
static luf                  f_ab_ab79_ab8_trgr;
static luf                      f_ab_ab79_ab8_trgr_flow;
static luf                      f_ab_ab79_ab8_trgr_prsr;
static luf                      f_ab_ab79_ab8_trgr_mass;
static luf                  f_ab_ab79_ab8_timeout;
static luf              f_ab_ab79_ab9;
static luf                  f_ab_ab79_ab9_ref;
static luf                      f_ab_ab79_ab9_ref_style;
static luf                      f_ab_ab79_ab9_ref_start;
static luf                      f_ab_ab79_ab9_ref_end;
static luf                  f_ab_ab79_ab9_trgr;
static luf                      f_ab_ab79_ab9_trgr_flow;
static luf                      f_ab_ab79_ab9_trgr_prsr;
static luf                      f_ab_ab79_ab9_trgr_mass;
static luf                  f_ab_ab79_ab9_timeout;
static luf      f_presets;
static luf          f_presets_profile_a;
static luf              f_presets_profile_a_0;
static luf              f_presets_profile_a_1;
static luf              f_presets_profile_a_2;
static luf          f_presets_profile_b;
static luf              f_presets_profile_b_0;
static luf              f_presets_profile_b_1;
static luf              f_presets_profile_b_2;
static luf          f_presets_profile_c;
static luf              f_presets_profile_c_0;
static luf              f_presets_profile_c_1;
static luf              f_presets_profile_c_2;

/**
 * \brief Returns the starting address of the given settings profile
 * \param id Profile number
 * \return Started memory address where profile is stored 
 */
static inline reg_addr _machine_settings_id_to_addr(uint8_t id){
    return MACHINE_SETTINGS_START_ADDR + (1+id)*MACHINE_SETTINGS_MEMORY_SIZE;
}

/**
 * \brief Restores settings to default state if any invalid values are found
 * 
 * \return true Invalid values found.
 * \return false No invalid values found.
 */
static bool _machine_settings_verify(){
    for(uint8_t p_id = 0; p_id < NUM_SETTINGS; p_id++){
        if(_ms[p_id] > _specs[p_id].max){
            // Invalid setting found. Copy in defaults and return true
            for(uint8_t p_id_2 = 0; p_id_2 < NUM_SETTINGS; p_id_2++){
                _ms[p_id_2] = _specs[p_id_2].std;
            }
            return true;
        }
    }
    return false;
}

/**
 * \brief Save the current settings array, \p _ms to the MB85 FRAM, \p _mem.
 * 
 * \param profile_id The number to save the profile under, 0 <= profile_id <= 8
 * \return PICO_ERROR_GENERIC if library not setup. Else PICO_ERROR_GENERIC.
 */
static int _machine_settings_save_profile(uint8_t profile_id){
    if(_mem == NULL) return PICO_ERROR_GENERIC;
    // Break link with profile buffer and connect to profile save address
    mb85_fram_unlink_var(_mem, &_ms);
    mb85_fram_link_var(_mem, &_ms, _machine_settings_id_to_addr(profile_id), MACHINE_SETTINGS_MEMORY_SIZE, MB85_FRAM_INIT_FROM_VAR);

    // Break link with save address and connect with profile buffer
    mb85_fram_unlink_var(_mem, &_ms);
    mb85_fram_link_var(_mem, &_ms, MACHINE_SETTINGS_START_ADDR, MACHINE_SETTINGS_MEMORY_SIZE, MB85_FRAM_INIT_FROM_VAR);
    return PICO_ERROR_NONE;
}

/**
 * \brief Load the indicated profile into the settings array, \p _ms from the MB85 FRAM, \p _mem.
 * 
 * \param profile_id The profile number to load, 0 <= profile_id <= 8
 * \return PICO_ERROR_GENERIC if library not setup. Else PICO_ERROR_GENERIC.
 */
static int _machine_settings_load_profile(uint8_t profile_id){
    if(_mem == NULL) return PICO_ERROR_GENERIC;
    // Break link with profile buffer and connect to profile load address
    mb85_fram_unlink_var(_mem, &_ms);
    mb85_fram_link_var(_mem, &_ms, _machine_settings_id_to_addr(profile_id), MACHINE_SETTINGS_MEMORY_SIZE, MB85_FRAM_INIT_FROM_FRAM);

    // Verify that loaded address is valid. If it wasn't, save back into profile.
    if(_machine_settings_verify()) mb85_fram_save(_mem, &_ms);

    // Break link with load address and connect with profile buffer
    mb85_fram_unlink_var(_mem, &_ms);
    mb85_fram_link_var(_mem, &_ms, MACHINE_SETTINGS_START_ADDR, MACHINE_SETTINGS_MEMORY_SIZE, MB85_FRAM_INIT_FROM_VAR);
    return PICO_ERROR_NONE;
}

/**
 * \brief Callback function used by setting action folders. Increments corresponding
 * setting or calls preset load/save function
 * 
 * \param id ID of calling folder
 * \param val Value of increment index. 0 = -10, 1 = +1, and 2 = +10
 * \returns True if value was greater than 2. False otherwise
 */
static bool _ms_f_cb(folder_id id, uint8_t val, folder_action_data ms_id){
    if (val > 2) return true;
    if (local_ui_id_in_subtree(&f_set, id) || local_ui_id_in_subtree(&f_ab, id) ){
        // Get step size (special case if base step size is 0)
        const int16_t deltas [3] = {-10*_specs[ms_id].step_size, _specs[ms_id].step_size, 10*_specs[ms_id].step_size};
        const int16_t step = (_specs[ms_id].step_size==0) ? val-1 : deltas[val];

        if(-deltas[val] > _ms[ms_id]){
            // If step would lead to negative value
            _ms[ms_id] = 0;
        } else {
            _ms[ms_id] = MIN(_ms[ms_id] + deltas[val], _specs[ms_id].max);
        }
        
        mb85_fram_save(_mem, _ms);
    } else if (local_ui_id_in_subtree(&f_presets, id)){
        // Presets
        if      (val == 0) _machine_settings_save_profile(ms_id);
        else if (val == 1) _machine_settings_load_profile(ms_id);
    }

    machine_settings_print();
    return false;
}

/** \brief Setup the local UI file structure. */
static void _machine_settings_setup_local_ui(){
    local_ui_folder_tree_init(&settings_modifier, &f_, "RaspberryLatte");
    local_ui_add_subfolder(&f_,                 &f_set,                   "Settings",                NULL, 0);
    local_ui_add_subfolder(&f_set,              &f_set_temp,              "Temperatures",            NULL, 0);
    local_ui_add_subfolder(&f_set_temp,         &f_set_temp_brew,         "Brew (-1, 0.1, 1)",       &_ms_f_cb, MS_TEMP_BREW_cC);
    local_ui_add_subfolder(&f_set_temp,         &f_set_temp_hot,          "Hot (-1, 0.1, 1)",        &_ms_f_cb, MS_TEMP_HOT_cC);
    local_ui_add_subfolder(&f_set_temp,         &f_set_temp_steam,        "Steam (-1, 0.1, 1)",      &_ms_f_cb, MS_TEMP_STEAM_cC);
    local_ui_add_subfolder(&f_set,              &f_set_weight,            "Weights",                 NULL, 0);
    local_ui_add_subfolder(&f_set_weight,       &f_set_weight_dose,       "Dose (-1, 0.1, 1)",       &_ms_f_cb, MS_WEIGHT_DOSE_mg);
    local_ui_add_subfolder(&f_set_weight,       &f_set_weight_yield,      "Yield (-1, 0.1, 1)",      &_ms_f_cb, MS_WEIGHT_YIELD_mg);
    local_ui_add_subfolder(&f_set,              &f_set_power,             "Power",                   NULL, 0);
    local_ui_add_subfolder(&f_set_power,        &f_set_power_brew,        "Brew (-10, 1, 10)",       &_ms_f_cb, MS_POWER_BREW_PER);
    local_ui_add_subfolder(&f_set_power,        &f_set_power_hot,         "Hot (-10, 1, 10)",        &_ms_f_cb, MS_POWER_HOT_PER);

    local_ui_add_subfolder(&f_,                 &f_ab,                    "Autobrew",                NULL, 0);
    local_ui_add_subfolder(&f_ab,               &f_ab_ab13,               "Autobrew Legs 1-3",       NULL, 0);
    local_ui_add_subfolder(&f_ab_ab13,          &f_ab_ab13_ab1,           "Autobrew Leg 1",          NULL, 0);
    local_ui_add_subfolder(&f_ab_ab13_ab1,      &f_ab_ab13_ab1_ref,       "Setpoint",                NULL, 0);
    local_ui_add_subfolder(&f_ab_ab13_ab1_ref,  &f_ab_ab13_ab1_ref_style, "Style (Pwr, Flow, Prsr)", _ms_f_cb, MS_A1_REF_STYLE_ENM);
    local_ui_add_subfolder(&f_ab_ab13_ab1_ref,  &f_ab_ab13_ab1_ref_start, "Starting Setpoint",       _ms_f_cb, MS_A1_REF_START_100per_ulps_mbar);
    local_ui_add_subfolder(&f_ab_ab13_ab1_ref,  &f_ab_ab13_ab1_ref_end,   "Ending Setpoint",         _ms_f_cb, MS_A1_REF_END_100per_ulps_mbar);
    local_ui_add_subfolder(&f_ab_ab13_ab1,      &f_ab_ab13_ab1_trgr,      "Trigger",                 NULL, 0);
    local_ui_add_subfolder(&f_ab_ab13_ab1_trgr, &f_ab_ab13_ab1_trgr_flow, "Flow (ml/s, -1, 0.1, 1)", _ms_f_cb, MS_A1_TRGR_FLOW_ul_s);
    local_ui_add_subfolder(&f_ab_ab13_ab1_trgr, &f_ab_ab13_ab1_trgr_prsr, "Prsr (bar, -1, 0.1, 1)",  _ms_f_cb, MS_A1_TRGR_PRSR_mbar);
    local_ui_add_subfolder(&f_ab_ab13_ab1_trgr, &f_ab_ab13_ab1_trgr_mass, "Mass (g, -1, 0.1, 1)",    _ms_f_cb, MS_A1_TRGR_MASS_mg);
    local_ui_add_subfolder(&f_ab_ab13_ab1,      &f_ab_ab13_ab1_timeout,   "Timeout (-1, 0.1, 1)",    _ms_f_cb, MS_A1_TIMEOUT_s);
    local_ui_add_subfolder(&f_ab_ab13,          &f_ab_ab13_ab2,           "Autobrew Leg 2",          NULL, 0);
    local_ui_add_subfolder(&f_ab_ab13_ab2,      &f_ab_ab13_ab2_ref,       "Setpoint",                NULL, 0);
    local_ui_add_subfolder(&f_ab_ab13_ab2_ref,  &f_ab_ab13_ab2_ref_style, "Style (Pwr, Flow, Prsr)", _ms_f_cb, MS_A2_REF_STYLE_ENM);
    local_ui_add_subfolder(&f_ab_ab13_ab2_ref,  &f_ab_ab13_ab2_ref_start, "Starting Setpoint",       _ms_f_cb, MS_A2_REF_START_100per_ulps_mbar);
    local_ui_add_subfolder(&f_ab_ab13_ab2_ref,  &f_ab_ab13_ab2_ref_end,   "Ending Setpoint",         _ms_f_cb, MS_A2_REF_END_100per_ulps_mbar);
    local_ui_add_subfolder(&f_ab_ab13_ab2,      &f_ab_ab13_ab2_trgr,      "Trigger",                 NULL, 0);
    local_ui_add_subfolder(&f_ab_ab13_ab2_trgr, &f_ab_ab13_ab2_trgr_flow, "Flow (ml/s, -1, 0.1, 1)", _ms_f_cb, MS_A2_TRGR_FLOW_ul_s);
    local_ui_add_subfolder(&f_ab_ab13_ab2_trgr, &f_ab_ab13_ab2_trgr_prsr, "Prsr (bar, -1, 0.1, 1)",  _ms_f_cb, MS_A2_TRGR_PRSR_mbar);
    local_ui_add_subfolder(&f_ab_ab13_ab2_trgr, &f_ab_ab13_ab2_trgr_mass, "Mass (g, -1, 0.1, 1)",    _ms_f_cb, MS_A2_TRGR_MASS_mg);
    local_ui_add_subfolder(&f_ab_ab13_ab2,      &f_ab_ab13_ab2_timeout,   "Timeout (-1, 0.1, 1)",    _ms_f_cb, MS_A2_TIMEOUT_s);
    local_ui_add_subfolder(&f_ab_ab13,          &f_ab_ab13_ab3,           "Autobrew Leg 3",          NULL, 0);
    local_ui_add_subfolder(&f_ab_ab13_ab3,      &f_ab_ab13_ab3_ref,       "Setpoint",                NULL, 0);
    local_ui_add_subfolder(&f_ab_ab13_ab3_ref,  &f_ab_ab13_ab3_ref_style, "Style (Pwr, Flow, Prsr)", _ms_f_cb, MS_A3_REF_STYLE_ENM);
    local_ui_add_subfolder(&f_ab_ab13_ab3_ref,  &f_ab_ab13_ab3_ref_start, "Starting Setpoint",       _ms_f_cb, MS_A3_REF_START_100per_ulps_mbar);
    local_ui_add_subfolder(&f_ab_ab13_ab3_ref,  &f_ab_ab13_ab3_ref_end,   "Ending Setpoint",         _ms_f_cb, MS_A3_REF_END_100per_ulps_mbar);
    local_ui_add_subfolder(&f_ab_ab13_ab3,      &f_ab_ab13_ab3_trgr,      "Trigger",                 NULL, 0);
    local_ui_add_subfolder(&f_ab_ab13_ab3_trgr, &f_ab_ab13_ab3_trgr_flow, "Flow (ml/s, -1, 0.1, 1)", _ms_f_cb, MS_A3_TRGR_FLOW_ul_s);
    local_ui_add_subfolder(&f_ab_ab13_ab3_trgr, &f_ab_ab13_ab3_trgr_prsr, "Prsr (bar, -1, 0.1, 1)",  _ms_f_cb, MS_A3_TRGR_PRSR_mbar);
    local_ui_add_subfolder(&f_ab_ab13_ab3_trgr, &f_ab_ab13_ab3_trgr_mass, "Mass (g, -1, 0.1, 1)",    _ms_f_cb, MS_A3_TRGR_MASS_mg);
    local_ui_add_subfolder(&f_ab_ab13_ab3,      &f_ab_ab13_ab3_timeout,   "Timeout (-1, 0.1, 1)",    _ms_f_cb, MS_A3_TIMEOUT_s);
    local_ui_add_subfolder(&f_ab,               &f_ab_ab46,               "Autobrew Legs 4-6",       NULL, 0);
    local_ui_add_subfolder(&f_ab_ab46,          &f_ab_ab46_ab4,           "Autobrew Leg 4",          NULL, 0);
    local_ui_add_subfolder(&f_ab_ab46_ab4,      &f_ab_ab46_ab4_ref,       "Setpoint",                NULL, 0);
    local_ui_add_subfolder(&f_ab_ab46_ab4_ref,  &f_ab_ab46_ab4_ref_style, "Style (Pwr, Flow, Prsr)", _ms_f_cb, MS_A4_REF_STYLE_ENM);
    local_ui_add_subfolder(&f_ab_ab46_ab4_ref,  &f_ab_ab46_ab4_ref_start, "Starting Setpoint",       _ms_f_cb, MS_A4_REF_START_100per_ulps_mbar);
    local_ui_add_subfolder(&f_ab_ab46_ab4_ref,  &f_ab_ab46_ab4_ref_end,   "Ending Setpoint",         _ms_f_cb, MS_A4_REF_END_100per_ulps_mbar);
    local_ui_add_subfolder(&f_ab_ab46_ab4,      &f_ab_ab46_ab4_trgr,      "Trigger",                 NULL, 0);
    local_ui_add_subfolder(&f_ab_ab46_ab4_trgr, &f_ab_ab46_ab4_trgr_flow, "Flow (ml/s, -1, 0.1, 1)", _ms_f_cb, MS_A4_TRGR_FLOW_ul_s);
    local_ui_add_subfolder(&f_ab_ab46_ab4_trgr, &f_ab_ab46_ab4_trgr_prsr, "Prsr (bar, -1, 0.1, 1)",  _ms_f_cb, MS_A4_TRGR_PRSR_mbar);
    local_ui_add_subfolder(&f_ab_ab46_ab4_trgr, &f_ab_ab46_ab4_trgr_mass, "Mass (g, -1, 0.1, 1)",    _ms_f_cb, MS_A4_TRGR_MASS_mg);
    local_ui_add_subfolder(&f_ab_ab46_ab4,      &f_ab_ab46_ab4_timeout,   "Timeout (-1, 0.1, 1)",    _ms_f_cb, MS_A4_TIMEOUT_s);
    local_ui_add_subfolder(&f_ab_ab46,          &f_ab_ab46_ab5,           "Autobrew Leg 5",          NULL, 0);
    local_ui_add_subfolder(&f_ab_ab46_ab5,      &f_ab_ab46_ab5_ref,       "Setpoint",                NULL, 0);
    local_ui_add_subfolder(&f_ab_ab46_ab5_ref,  &f_ab_ab46_ab5_ref_style, "Style (Pwr, Flow, Prsr)", _ms_f_cb, MS_A5_REF_STYLE_ENM);
    local_ui_add_subfolder(&f_ab_ab46_ab5_ref,  &f_ab_ab46_ab5_ref_start, "Starting Setpoint",       _ms_f_cb, MS_A5_REF_START_100per_ulps_mbar);
    local_ui_add_subfolder(&f_ab_ab46_ab5_ref,  &f_ab_ab46_ab5_ref_end,   "Ending Setpoint",         _ms_f_cb, MS_A5_REF_END_100per_ulps_mbar);
    local_ui_add_subfolder(&f_ab_ab46_ab5,      &f_ab_ab46_ab5_trgr,      "Trigger",                 NULL, 0);
    local_ui_add_subfolder(&f_ab_ab46_ab5_trgr, &f_ab_ab46_ab5_trgr_flow, "Flow (ml/s, -1, 0.1, 1)", _ms_f_cb, MS_A5_TRGR_FLOW_ul_s);
    local_ui_add_subfolder(&f_ab_ab46_ab5_trgr, &f_ab_ab46_ab5_trgr_prsr, "Prsr (bar, -1, 0.1, 1)",  _ms_f_cb, MS_A5_TRGR_PRSR_mbar);
    local_ui_add_subfolder(&f_ab_ab46_ab5_trgr, &f_ab_ab46_ab5_trgr_mass, "Mass (g, -1, 0.1, 1)",    _ms_f_cb, MS_A5_TRGR_MASS_mg);
    local_ui_add_subfolder(&f_ab_ab46_ab5,      &f_ab_ab46_ab5_timeout,   "Timeout (-1, 0.1, 1)",    _ms_f_cb, MS_A5_TIMEOUT_s);
    local_ui_add_subfolder(&f_ab_ab46,          &f_ab_ab46_ab6,           "Autobrew Leg 6",          NULL, 0);
    local_ui_add_subfolder(&f_ab_ab46_ab6,      &f_ab_ab46_ab6_ref,       "Setpoint",                NULL, 0);
    local_ui_add_subfolder(&f_ab_ab46_ab6_ref,  &f_ab_ab46_ab6_ref_style, "Style (Pwr, Flow, Prsr)", _ms_f_cb, MS_A6_REF_STYLE_ENM);
    local_ui_add_subfolder(&f_ab_ab46_ab6_ref,  &f_ab_ab46_ab6_ref_start, "Starting Setpoint",       _ms_f_cb, MS_A6_REF_START_100per_ulps_mbar);
    local_ui_add_subfolder(&f_ab_ab46_ab6_ref,  &f_ab_ab46_ab6_ref_end,   "Ending Setpoint",         _ms_f_cb, MS_A6_REF_END_100per_ulps_mbar);
    local_ui_add_subfolder(&f_ab_ab46_ab6,      &f_ab_ab46_ab6_trgr,      "Trigger",                 NULL, 0);
    local_ui_add_subfolder(&f_ab_ab46_ab6_trgr, &f_ab_ab46_ab6_trgr_flow, "Flow (ml/s, -1, 0.1, 1)", _ms_f_cb, MS_A6_TRGR_FLOW_ul_s);
    local_ui_add_subfolder(&f_ab_ab46_ab6_trgr, &f_ab_ab46_ab6_trgr_prsr, "Prsr (bar, -1, 0.1, 1)",  _ms_f_cb, MS_A6_TRGR_PRSR_mbar);
    local_ui_add_subfolder(&f_ab_ab46_ab6_trgr, &f_ab_ab46_ab6_trgr_mass, "Mass (g, -1, 0.1, 1)",    _ms_f_cb, MS_A6_TRGR_MASS_mg);
    local_ui_add_subfolder(&f_ab_ab46_ab6,      &f_ab_ab46_ab6_timeout,   "Timeout (-1, 0.1, 1)",    _ms_f_cb, MS_A6_TIMEOUT_s);
    local_ui_add_subfolder(&f_ab,               &f_ab_ab79,               "Autobrew Legs 4-6",       NULL, 0);
    local_ui_add_subfolder(&f_ab_ab79,          &f_ab_ab79_ab7,           "Autobrew Leg 7",          NULL, 0);
    local_ui_add_subfolder(&f_ab_ab79_ab7,      &f_ab_ab79_ab7_ref,       "Setpoint",                NULL, 0);
    local_ui_add_subfolder(&f_ab_ab79_ab7_ref,  &f_ab_ab79_ab7_ref_style, "Style (Pwr, Flow, Prsr)", _ms_f_cb, MS_A4_REF_STYLE_ENM);
    local_ui_add_subfolder(&f_ab_ab79_ab7_ref,  &f_ab_ab79_ab7_ref_start, "Starting Setpoint",       _ms_f_cb, MS_A4_REF_START_100per_ulps_mbar);
    local_ui_add_subfolder(&f_ab_ab79_ab7_ref,  &f_ab_ab79_ab7_ref_end,   "Ending Setpoint",         _ms_f_cb, MS_A4_REF_END_100per_ulps_mbar);
    local_ui_add_subfolder(&f_ab_ab79_ab7,      &f_ab_ab79_ab7_trgr,      "Trigger",                 NULL, 0);
    local_ui_add_subfolder(&f_ab_ab79_ab7_trgr, &f_ab_ab79_ab7_trgr_flow, "Flow (ml/s, -1, 0.1, 1)", _ms_f_cb, MS_A4_TRGR_FLOW_ul_s);
    local_ui_add_subfolder(&f_ab_ab79_ab7_trgr, &f_ab_ab79_ab7_trgr_prsr, "Prsr (bar, -1, 0.1, 1)",  _ms_f_cb, MS_A4_TRGR_PRSR_mbar);
    local_ui_add_subfolder(&f_ab_ab79_ab7_trgr, &f_ab_ab79_ab7_trgr_mass, "Mass (g, -1, 0.1, 1)",    _ms_f_cb, MS_A4_TRGR_MASS_mg);
    local_ui_add_subfolder(&f_ab_ab79_ab7,      &f_ab_ab79_ab7_timeout,   "Timeout (-1, 0.1, 1)",    _ms_f_cb, MS_A4_TIMEOUT_s);
    local_ui_add_subfolder(&f_ab_ab79,          &f_ab_ab79_ab8,           "Autobrew Leg 8",          NULL, 0);
    local_ui_add_subfolder(&f_ab_ab79_ab8,      &f_ab_ab79_ab8_ref,       "Setpoint",                NULL, 0);
    local_ui_add_subfolder(&f_ab_ab79_ab8_ref,  &f_ab_ab79_ab8_ref_style, "Style (Pwr, Flow, Prsr)", _ms_f_cb, MS_A5_REF_STYLE_ENM);
    local_ui_add_subfolder(&f_ab_ab79_ab8_ref,  &f_ab_ab79_ab8_ref_start, "Starting Setpoint",       _ms_f_cb, MS_A5_REF_START_100per_ulps_mbar);
    local_ui_add_subfolder(&f_ab_ab79_ab8_ref,  &f_ab_ab79_ab8_ref_end,   "Ending Setpoint",         _ms_f_cb, MS_A5_REF_END_100per_ulps_mbar);
    local_ui_add_subfolder(&f_ab_ab79_ab8,      &f_ab_ab79_ab8_trgr,      "Trigger",                 NULL, 0);
    local_ui_add_subfolder(&f_ab_ab79_ab8_trgr, &f_ab_ab79_ab8_trgr_flow, "Flow (ml/s, -1, 0.1, 1)", _ms_f_cb, MS_A5_TRGR_FLOW_ul_s);
    local_ui_add_subfolder(&f_ab_ab79_ab8_trgr, &f_ab_ab79_ab8_trgr_prsr, "Prsr (bar, -1, 0.1, 1)",  _ms_f_cb, MS_A5_TRGR_PRSR_mbar);
    local_ui_add_subfolder(&f_ab_ab79_ab8_trgr, &f_ab_ab79_ab8_trgr_mass, "Mass (g, -1, 0.1, 1)",    _ms_f_cb, MS_A5_TRGR_MASS_mg);
    local_ui_add_subfolder(&f_ab_ab79_ab8,      &f_ab_ab79_ab8_timeout,   "Timeout (-1, 0.1, 1)",    _ms_f_cb, MS_A5_TIMEOUT_s);
    local_ui_add_subfolder(&f_ab_ab79,          &f_ab_ab79_ab9,           "Autobrew Leg 9",          NULL, 0);
    local_ui_add_subfolder(&f_ab_ab79_ab9,      &f_ab_ab79_ab9_ref,       "Setpoint",                NULL, 0);
    local_ui_add_subfolder(&f_ab_ab79_ab9_ref,  &f_ab_ab79_ab9_ref_style, "Style (Pwr, Flow, Prsr)", _ms_f_cb, MS_A6_REF_STYLE_ENM);
    local_ui_add_subfolder(&f_ab_ab79_ab9_ref,  &f_ab_ab79_ab9_ref_start, "Starting Setpoint",       _ms_f_cb, MS_A6_REF_START_100per_ulps_mbar);
    local_ui_add_subfolder(&f_ab_ab79_ab9_ref,  &f_ab_ab79_ab9_ref_end,   "Ending Setpoint",         _ms_f_cb, MS_A6_REF_END_100per_ulps_mbar);
    local_ui_add_subfolder(&f_ab_ab79_ab9,      &f_ab_ab79_ab9_trgr,      "Trigger",                 NULL, 0);
    local_ui_add_subfolder(&f_ab_ab79_ab9_trgr, &f_ab_ab79_ab9_trgr_flow, "Flow (ml/s, -1, 0.1, 1)", _ms_f_cb, MS_A6_TRGR_FLOW_ul_s);
    local_ui_add_subfolder(&f_ab_ab79_ab9_trgr, &f_ab_ab79_ab9_trgr_prsr, "Prsr (bar, -1, 0.1, 1)",  _ms_f_cb, MS_A6_TRGR_PRSR_mbar);
    local_ui_add_subfolder(&f_ab_ab79_ab9_trgr, &f_ab_ab79_ab9_trgr_mass, "Mass (g, -1, 0.1, 1)",    _ms_f_cb, MS_A6_TRGR_MASS_mg);
    local_ui_add_subfolder(&f_ab_ab79_ab9,      &f_ab_ab79_ab9_timeout,   "Timeout (-1, 0.1, 1)",    _ms_f_cb, MS_A6_TIMEOUT_s);

    local_ui_add_subfolder(&f_,                 &f_presets,               "Presets",                 NULL, 0);
    local_ui_add_subfolder(&f_presets,          &f_presets_profile_a,     "Presets 1-3",             NULL, 0);
    local_ui_add_subfolder(&f_presets_profile_a,&f_presets_profile_a_0,   "Preset 1 (save, load)",   &_ms_f_cb, 0);
    local_ui_add_subfolder(&f_presets_profile_a,&f_presets_profile_a_1,   "Preset 2 (save, load)",   &_ms_f_cb, 1);
    local_ui_add_subfolder(&f_presets_profile_a,&f_presets_profile_a_2,   "Preset 3 (save, load)",   &_ms_f_cb, 2);
    local_ui_add_subfolder(&f_presets,          &f_presets_profile_b,     "Presets 4-6",             NULL, 0);
    local_ui_add_subfolder(&f_presets_profile_b,&f_presets_profile_b_0,   "Preset 4 (save, load)",   &_ms_f_cb, 3);
    local_ui_add_subfolder(&f_presets_profile_b,&f_presets_profile_b_1,   "Preset 5 (save, load)",   &_ms_f_cb, 4);
    local_ui_add_subfolder(&f_presets_profile_b,&f_presets_profile_b_2,   "Preset 6 (save, load)",   &_ms_f_cb, 5);
    local_ui_add_subfolder(&f_presets,          &f_presets_profile_c,     "Presets 7-9",             NULL, 0);
    local_ui_add_subfolder(&f_presets_profile_c,&f_presets_profile_c_0,   "Preset 7 (save, load)",   &_ms_f_cb, 6);
    local_ui_add_subfolder(&f_presets_profile_c,&f_presets_profile_c_1,   "Preset 8 (save, load)",   &_ms_f_cb, 7);
    local_ui_add_subfolder(&f_presets_profile_c,&f_presets_profile_c_2,   "Preset 9 (save, load)",   &_ms_f_cb, 8);
}

void machine_settings_setup(mb85_fram mem){
    if(_mem == NULL){
        _mem = mem;
        if(mb85_fram_link_var(_mem, &_ms, MACHINE_SETTINGS_START_ADDR, MACHINE_SETTINGS_MEMORY_SIZE, MB85_FRAM_INIT_FROM_FRAM)){
            _mem = NULL;
            return;
        }
        if(_machine_settings_verify()){
            // If settings had to be reset to defaults, save new values.
            mb85_fram_save(_mem, &_ms);
        }
        _machine_settings_setup_local_ui();

        // Create value_flasher object
        _setting_flasher = value_flasher_setup(0, 750, &_ui_mask);
    }
}

machine_setting machine_settings_get(setting_id id){
    assert(id < NUM_SETTINGS || id == MS_UI_MASK);
    if(_mem == NULL) return 0; // nothing setup yet :(
    else if (id < NUM_SETTINGS) return _ms[id];
    else return _ui_mask;
}

int machine_settings_update(bool reset, bool select, uint8_t val){
    if (reset){
        local_ui_go_to_root(&settings_modifier);
        value_flasher_end(_setting_flasher);
        _ui_mask = 0;
    } else if (select){
        if (val == 3){
            // Return to root
            local_ui_go_to_root(&settings_modifier);
            value_flasher_end(_setting_flasher);
            _ui_mask = 0;
        } else {
            local_ui_enter_subfolder(&settings_modifier, 2 - val);
            
            const folder_id id = settings_modifier.cur_folder->id;
            if(local_ui_is_action_folder(settings_modifier.cur_folder) &&
                (local_ui_id_in_subtree(&f_set, id) || local_ui_id_in_subtree(&f_ab, id))){
                // If entered action settings folder, start value flasher
                 value_flasher_update(_setting_flasher, _ms[settings_modifier.cur_folder->data]);
                 value_flasher_start(_setting_flasher);
            } else {
                // else in nav folder. Display id.
                value_flasher_end(_setting_flasher);
                _ui_mask = settings_modifier.cur_folder->rel_id;
            }
        }
    }
    return PICO_ERROR_NONE;
}


int machine_settings_print(){
    if(_mem == NULL) return PICO_ERROR_GENERIC;
    printf(
        "Brew temp          : %0.2f C\n"
        "Hot temp           : %0.2f C\n"
        "Steam temp         : %0.2f C\n"
        "Dose               : %0.2f g\n"
        "Yield              : %0.2f g\n"
        "Brew power         : %u%%\n"
        "Hot power          : %u%%\n\n",
        _ms[MS_TEMP_BREW_cC]/1000.,
        _ms[MS_TEMP_HOT_cC]/1000.,
        _ms[MS_TEMP_STEAM_cC]/1000.,
        _ms[MS_WEIGHT_DOSE_mg]/1000.,
        _ms[MS_WEIGHT_YIELD_mg]/1000.,
        _ms[MS_POWER_BREW_PER],
        _ms[MS_POWER_HOT_PER]);
    
    printf("______________________________________________________________\n");
    printf("|        Setpoint         |         Target         | Timeout |\n");
    printf("|  Style  : Start :  End  | Flow : Pressure : Mass |         |\n");
    printf("|---------:-------:-------|------:----------:------|---------|\n");
    for(uint8_t i = 0; i < NUM_AUTOBREW_LEGS; i++){
        const uint8_t offset = i*NUM_AUTOBREW_PARAMS_PER_LEG;
        printf(
           "|%s: %5.1f : %5.1f | %4.1f :   %4.1f   : %4.1f |   %2u    |\n",
            (_ms[offset + MS_A1_REF_STYLE_ENM] == 0 ? "  Power  " : (_ms[offset+MS_A1_REF_STYLE_ENM] == 1 ? "  Flow   " : " Pressure")),
            _ms[offset + MS_A1_REF_START_100per_ulps_mbar]/(_ms[offset+MS_A1_REF_STYLE_ENM] == 0 ? 100.0 : 1000.),
            _ms[offset + MS_A1_REF_END_100per_ulps_mbar]/(_ms[offset+MS_A1_REF_STYLE_ENM] == 0 ? 100.0 : 1000.),
            _ms[offset + MS_A1_TRGR_FLOW_ul_s]/1000.,
            _ms[offset + MS_A1_TRGR_PRSR_mbar]/1000.,
            _ms[offset + MS_A1_TRGR_MASS_mg]/1000.,
            _ms[offset + MS_A1_TIMEOUT_s]);
    }
    printf("|---------:-------:-------|------:----------:------|---------|\n\n");
    return PICO_ERROR_NONE;
}

/** @} */