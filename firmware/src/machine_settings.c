#include "machine_settings.h"

#include <stdio.h>
#include <string.h>

#include "local_ui.h"
#include "value_flasher.h"

/** \brief Enumerated list naming the indicies of the settings array. */
typedef enum {TEMP_BREW = 0,  TEMP_HOT,        TEMP_STEAM,      WEIGHT_DOSE, WEIGHT_YIELD,
              PREINF_ON_TIME, PREINF_OFF_TIME, PREINF_ON_POWER, RAMP_TIME,   TIMEOUT,
              POWER_BREW,     POWER_HOT,       NUM_SETTINGS} setting_id;

/** \brief Starting address in mb85 FRAM chip where setting data is stored */
static const reg_addr MACHINE_SETTINGS_START_ADDR = 0x0000;
/** \brief The size, in bytes, of a single settings profile */
static const uint16_t MACHINE_SETTINGS_MEMORY_SIZE = NUM_SETTINGS * sizeof(machine_setting);

/** \brief Pointer to FRAM memory IC object where settings are stored */
static mb85_fram * _mem = NULL;
/** \brief Flasher object to display a setting when it's getting modified. */
static value_flasher _setting_flasher;

/** \brief Internal settings array holding the master settings */
static machine_setting _ms [NUM_SETTINGS];
/** \brief Internal settings structure with fields pointing at \p _ms array */
static machine_settings _ms_struct;
/** \brief Minimum settings array */
static const machine_setting _ms_min [NUM_SETTINGS] = {   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0};
/** \brief Maximum settings array */
static const machine_setting _ms_max [NUM_SETTINGS] = {1450, 1450, 1450, 500, 1000, 600, 600, 100, 600, 180, 100, 100};
/** \brief Default settings array */
static const machine_setting _ms_std [NUM_SETTINGS] = { 900, 1000, 1450, 150,  300,  40,  40,  75,  10,  60, 100,  75};

static local_ui_folder_tree settings_modifier; /**< \brief Local UI folder tree for updating machine settings*/
static local_ui_folder folder_root; /**< \brief Local UI folder: / */
static local_ui_folder folder_settings; /**< \brief Local UI folder: /settings/ */
static local_ui_folder folder_settings_temp; /**< \brief Local UI folder: /settings/temp/ */
static local_ui_folder folder_settings_temp_brew; /**< \brief Local UI folder: /settings/temp/brew/ */
static local_ui_folder folder_settings_temp_hot; /**< \brief Local UI folder: /settings/temp/hot/ */
static local_ui_folder folder_settings_temp_steam; /**< \brief Local UI folder: /settings/temp/steam/ */
static local_ui_folder folder_settings_weight; /**< \brief Local UI folder: /settings/weight/ */
static local_ui_folder folder_settings_weight_yield; /**< \brief Local UI folder: /settings/weight/yield/ */
static local_ui_folder folder_settings_weight_dose; /**< \brief Local UI folder: /settings/weight/dose/ */
static local_ui_folder folder_settings_more; /**< \brief Local UI folder: /settings/more/ */
static local_ui_folder folder_settings_more_power; /**< \brief Local UI folder: /settings/more/power/ */
static local_ui_folder folder_settings_more_power_brew; /**< \brief Local UI folder: /settings/more/power/brew/ */
static local_ui_folder folder_settings_more_power_hot; /**< \brief Local UI folder: /settings/more/power/hot/ */
static local_ui_folder folder_settings_more_preinfuse; /**< \brief Local UI folder: /settings/more/preinfuse/ */
static local_ui_folder folder_settings_more_preinfuse_on_time; /**< \brief Local UI folder: /settings/more/preinfuse/on_time/ */
static local_ui_folder folder_settings_more_preinfuse_on_power; /**< \brief Local UI folder: /settings/more/preinfuse/on_power/ */
static local_ui_folder folder_settings_more_preinfuse_off_time; /**< \brief Local UI folder: /settings/more/preinfuse/off_time/ */
static local_ui_folder folder_settings_more_misc; /**< \brief Local UI folder: /settings/more/misc/ */
static local_ui_folder folder_settings_more_misc_timeout; /**< \brief Local UI folder: /settings/more/misc/timeout/ */
static local_ui_folder folder_settings_more_misc_ramp_time; /**< \brief Local UI folder: /settings/more/misc/ramp_time/ */
static local_ui_folder folder_presets; /**< \brief Local UI folder: /presets/ */
static local_ui_folder folder_presets_profile_a; /**< \brief Local UI folder: /presets/profile_a/ */
static local_ui_folder folder_presets_profile_a_0; /**< \brief Local UI folder: /presets/profile_a/0/ */
static local_ui_folder folder_presets_profile_a_1; /**< \brief Local UI folder: /presets/profile_a/1/ */
static local_ui_folder folder_presets_profile_a_2; /**< \brief Local UI folder: /presets/profile_a/2/ */
static local_ui_folder folder_presets_profile_b; /**< \brief Local UI folder: /presets/profile_b/ */
static local_ui_folder folder_presets_profile_b_0; /**< \brief Local UI folder: /presets/profile_b/0/ */
static local_ui_folder folder_presets_profile_b_1; /**< \brief Local UI folder: /presets/profile_b/1/ */
static local_ui_folder folder_presets_profile_b_2; /**< \brief Local UI folder: /presets/profile_b/2/ */
static local_ui_folder folder_presets_profile_c; /**< \brief Local UI folder: /presets/profile_c/ */
static local_ui_folder folder_presets_profile_c_0; /**< \brief Local UI folder: /presets/profile_c/0/ */
static local_ui_folder folder_presets_profile_c_1; /**< \brief Local UI folder: /presets/profile_c/1/ */
static local_ui_folder folder_presets_profile_c_2; /**< \brief Local UI folder: /presets/profile_c/2/ */

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
        if(_ms[p_id] < _ms_min[p_id] || _ms[p_id] > _ms_max[p_id]){
            // Invalid setting found. Copy in defaults and return true
            memcpy(_ms, _ms_std, MACHINE_SETTINGS_MEMORY_SIZE);
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
 * \brief Convert a folder ID to the correct setting or profile ID
 * 
 * \param id Folder ID
 * \return int8_t the folder or profile ID or -1 if none found.
 */
static int8_t _machine_settings_folder_to_setting(folder_id id){
    if (local_ui_id_in_subtree(&folder_settings, id)){
        if (local_ui_id_in_subtree(&folder_settings_temp, id)){
            if (id == folder_settings_temp_brew.id)                        return TEMP_BREW;
            else if (id == folder_settings_temp_hot.id)                    return TEMP_HOT;
            else if (id == folder_settings_temp_steam.id)                  return TEMP_STEAM;
        } else if (local_ui_id_in_subtree(&folder_settings_weight, id)){
            if (id == folder_settings_weight_dose.id)                      return WEIGHT_DOSE;
            else if (id == folder_settings_weight_yield.id)                return WEIGHT_YIELD;
        } else if (local_ui_id_in_subtree(&folder_settings_more, id)){
            if (local_ui_id_in_subtree(&folder_settings_more_power, id)){
                if (id == folder_settings_more_power_brew.id)              return POWER_BREW;
                else if (id == folder_settings_more_power_hot.id)          return POWER_HOT;
            } else if (local_ui_id_in_subtree(&folder_settings_more_preinfuse, id)){
                // Settings/more/preinfuse
                if (id == folder_settings_more_preinfuse_on_time.id)       return PREINF_ON_TIME;
                else if (id == folder_settings_more_preinfuse_off_time.id) return PREINF_OFF_TIME;
                else if (id == folder_settings_more_preinfuse_on_power.id) return PREINF_ON_POWER;
            } else if (local_ui_id_in_subtree(&folder_settings_more_misc, id)){
                // Settings/more/misc
                if (id == folder_settings_more_misc_timeout.id)            return TIMEOUT;
                else if (id == folder_settings_more_misc_ramp_time.id)     return RAMP_TIME;
            }
        }
    } else if (local_ui_id_in_subtree(&folder_presets, id)){
        if (local_ui_id_in_subtree(&folder_presets_profile_a, id)){
            if (id == folder_presets_profile_a_0.id)      return 0;
            else if (id == folder_presets_profile_a_1.id) return 1;
            else if (id == folder_presets_profile_a_2.id) return 2;
        } else if (local_ui_id_in_subtree(&folder_presets_profile_b, id)){
            // Presets 4-6
            if (id == folder_presets_profile_b_0.id)      return 3;
            else if (id == folder_presets_profile_b_1.id) return 4;
            else if (id == folder_presets_profile_b_2.id) return 5;
        } else if (local_ui_id_in_subtree(&folder_presets_profile_c, id)){
            // Presets 7-9
            if (id == folder_presets_profile_c_0.id)      return 6;
            else if (id == folder_presets_profile_c_1.id) return 7;
            else if (id == folder_presets_profile_c_2.id) return 8;
        }
    }
    return -1;
}

/**
 * \brief Callback function used by setting action folders. Increments corresponding
 * setting or calls preset load/save function
 * 
 * \param id ID of calling folder
 * \param val Value of increment index. 0 = -10, 1 = +1, and 2 = +10
 * \returns True if value was greater than 2. False otherwise
 */
static bool _machine_settings_folder_callback(folder_id id, uint8_t val){
    if (val > 2) return true;
    if (local_ui_id_in_subtree(&folder_settings, id)){
        // Settings
        const int8_t deltas [] = {-10, 1, 10};
        const setting_id ms_id = _machine_settings_folder_to_setting(id);
        if (ms_id == -1) return true;

        // Add delta and clip if needed
        _ms[ms_id] += deltas[val];
        if (_ms[ms_id] < _ms_min[ms_id]) _ms[ms_id] = _ms_min[ms_id];
        else if (_ms[ms_id] > _ms_max[ms_id]) _ms[ms_id] = _ms_max[ms_id];
    } else if (local_ui_id_in_subtree(&folder_presets, id)){
        // Presets
        const int8_t profile_id = _machine_settings_folder_to_setting(id);
        if (profile_id == -1) return true;
        if      (val == 0) _machine_settings_save_profile(profile_id);
        else if (val == 1) _machine_settings_load_profile(profile_id);
    }

    machine_settings_print();
    return false;
}

/** \brief Link the internal settings array with the externally accessible settings struct */
static void _machine_settings_link(){
    _ms_struct.autobrew.preinf_off_time  = &(_ms[PREINF_OFF_TIME]);
    _ms_struct.autobrew.preinf_on_time   = &(_ms[PREINF_ON_TIME]);
    _ms_struct.autobrew.preinf_power     = &(_ms[PREINF_ON_POWER]);
    _ms_struct.autobrew.timeout          = &(_ms[TIMEOUT]);
    _ms_struct.autobrew.preinf_ramp_time = &(_ms[RAMP_TIME]);
    _ms_struct.brew.temp                 = &(_ms[TEMP_BREW]);
    _ms_struct.hot.temp                  = &(_ms[TEMP_HOT]);
    _ms_struct.steam.temp                = &(_ms[TEMP_STEAM]);
    _ms_struct.brew.power                = &(_ms[POWER_BREW]);
    _ms_struct.hot.power                 = &(_ms[POWER_HOT]);
    _ms_struct.brew.dose                 = &(_ms[WEIGHT_DOSE]);
    _ms_struct.brew.yield                = &(_ms[WEIGHT_YIELD]);
}

/** \brief Setup the local UI file structure. */
static void _machine_settings_setup_local_ui(){
    local_ui_folder_tree_init(&settings_modifier, &folder_root, "RaspberryLatte");
    local_ui_add_subfolder(&folder_root,                    &folder_settings,                         "Settings",                  NULL);
    local_ui_add_subfolder(&folder_settings,                &folder_settings_temp,                    "Temperatures",              NULL);
    local_ui_add_subfolder(&folder_settings_temp,           &folder_settings_temp_brew,               "Brew (-1, 0.1, 1)",         &_machine_settings_folder_callback);
    local_ui_add_subfolder(&folder_settings_temp,           &folder_settings_temp_hot,                "Hot (-1, 0.1, 1)",          &_machine_settings_folder_callback);
    local_ui_add_subfolder(&folder_settings_temp,           &folder_settings_temp_steam,              "Steam (-1, 0.1, 1)",        &_machine_settings_folder_callback);
    local_ui_add_subfolder(&folder_settings,                &folder_settings_weight,                  "Weights",                   NULL);
    local_ui_add_subfolder(&folder_settings_weight,         &folder_settings_weight_dose,             "Dose (-1, 0.1, 1)",         &_machine_settings_folder_callback);
    local_ui_add_subfolder(&folder_settings_weight,         &folder_settings_weight_yield,            "Yield (-1, 0.1, 1)",        &_machine_settings_folder_callback);
    local_ui_add_subfolder(&folder_settings,                &folder_settings_more,                    "More",                      NULL);
    local_ui_add_subfolder(&folder_settings_more,           &folder_settings_more_power,              "Power",                     NULL);
    local_ui_add_subfolder(&folder_settings_more_power,     &folder_settings_more_power_brew,         "Brew (-10, 1, 10)",         &_machine_settings_folder_callback);
    local_ui_add_subfolder(&folder_settings_more_power,     &folder_settings_more_power_hot,          "Hot (-10, 1, 10)",          &_machine_settings_folder_callback);
    local_ui_add_subfolder(&folder_settings_more,           &folder_settings_more_preinfuse,          "Preinfuse",                 NULL);
    local_ui_add_subfolder(&folder_settings_more_preinfuse, &folder_settings_more_preinfuse_on_time,  "On Time (-1, 0.1, 1)",      &_machine_settings_folder_callback);
    local_ui_add_subfolder(&folder_settings_more_preinfuse, &folder_settings_more_preinfuse_on_power, "On Power (-10, 1, 10)",     &_machine_settings_folder_callback);
    local_ui_add_subfolder(&folder_settings_more_preinfuse, &folder_settings_more_preinfuse_off_time, "Off Time (-1, 0.1, 1)",     &_machine_settings_folder_callback);
    local_ui_add_subfolder(&folder_settings_more,           &folder_settings_more_misc,               "Misc",                      NULL);
    local_ui_add_subfolder(&folder_settings_more_misc,      &folder_settings_more_misc_timeout,       "Timeout (-10, 1, 10)",      &_machine_settings_folder_callback);
    local_ui_add_subfolder(&folder_settings_more_misc,      &folder_settings_more_misc_ramp_time,     "Ramp Time (-1, 0.1, 1)",    &_machine_settings_folder_callback);
    local_ui_add_subfolder(&folder_root,                    &folder_presets,                          "Presets",                   NULL);
    local_ui_add_subfolder(&folder_presets,                 &folder_presets_profile_a,                "Presets 1-3",               NULL);
    local_ui_add_subfolder(&folder_presets_profile_a,       &folder_presets_profile_a_0,              "Preset 1 (save, load)",     &_machine_settings_folder_callback);
    local_ui_add_subfolder(&folder_presets_profile_a,       &folder_presets_profile_a_1,              "Preset 2 (save, load)",     &_machine_settings_folder_callback);
    local_ui_add_subfolder(&folder_presets_profile_a,       &folder_presets_profile_a_2,              "Preset 3 (save, load)",     &_machine_settings_folder_callback);
    local_ui_add_subfolder(&folder_presets,                 &folder_presets_profile_b,                "Presets 4-6",               NULL);
    local_ui_add_subfolder(&folder_presets_profile_b,       &folder_presets_profile_b_0,              "Preset 4 (save, load)",     &_machine_settings_folder_callback);
    local_ui_add_subfolder(&folder_presets_profile_b,       &folder_presets_profile_b_1,              "Preset 5 (save, load)",     &_machine_settings_folder_callback);
    local_ui_add_subfolder(&folder_presets_profile_b,       &folder_presets_profile_b_2,              "Preset 6 (save, load)",     &_machine_settings_folder_callback);
    local_ui_add_subfolder(&folder_presets,                 &folder_presets_profile_c,                "Presets 7-9",               NULL);
    local_ui_add_subfolder(&folder_presets_profile_c,       &folder_presets_profile_c_0,              "Preset 7 (save, load)",     &_machine_settings_folder_callback);
    local_ui_add_subfolder(&folder_presets_profile_c,       &folder_presets_profile_c_1,              "Preset 8 (save, load)",     &_machine_settings_folder_callback);
    local_ui_add_subfolder(&folder_presets_profile_c,       &folder_presets_profile_c_2,              "Preset 9 (save, load)",     &_machine_settings_folder_callback);
}

const machine_settings * machine_settings_setup(mb85_fram * mem){
    if(_mem == NULL){
        _mem = mem;
        if(mb85_fram_link_var(_mem, &_ms, MACHINE_SETTINGS_START_ADDR, MACHINE_SETTINGS_MEMORY_SIZE, MB85_FRAM_INIT_FROM_FRAM)){
            return NULL;
        }
        if(_machine_settings_verify()){
            // If settings had to be reset to defaults, save new values.
            mb85_fram_save(_mem, &_ms);
        }
        _machine_settings_link();
        _machine_settings_setup_local_ui();
    }
    return &_ms_struct;
}

const machine_settings * machine_settings_acquire(){
    if(_mem == NULL) return NULL;
    else return &_ms_struct;
}

int machine_settings_update(bool reset, bool select, uint8_t val){
    if (reset){
        local_ui_go_to_root(&settings_modifier);
        value_flasher_end(&_setting_flasher);
        _ms_struct.ui_mask = 0;
    } else if (select){
        if (val == 3){
            // Return to root
            local_ui_go_to_root(&settings_modifier);
            value_flasher_end(&_setting_flasher);
            _ms_struct.ui_mask = 0;
        } else {
            local_ui_enter_subfolder(&settings_modifier, 2 - val);
            
            const folder_id id = settings_modifier.cur_folder->id;
            if(local_ui_is_action_folder(settings_modifier.cur_folder) &&
                local_ui_id_in_subtree(&folder_settings, id)){
                // If entered action settings folder, start value flasher
                value_flasher_setup(&_setting_flasher, _ms[_machine_settings_folder_to_setting(id)], 750, &_ms_struct.ui_mask);
            } else {
                // else in nav folder. Display id.
                value_flasher_end(&_setting_flasher);
                _ms_struct.ui_mask = 3 - val;
            }
        }
    }
}


int machine_settings_print(){
    if(_mem == NULL) return PICO_ERROR_GENERIC;
    printf(
        "Preinfuse on time  : %0.2fs\n"
        "Preinfuse off time : %0.2fs\n"
        "Timeout length     : %ds\n"
        "Ramp length        : %0.2fs\n"
        "Dose               : %0.2fg\n"
        "Yield              : %0.2fg\n"
        "Brew temp          : %0.2fC\n"
        "Hot temp           : %0.2fC\n"
        "Steam temp         : %0.2fC\n"
        "Preinfuse power    : %d\%\n"
        "Brew power         : %d\%\n"
        "Hot power          : %d\%\n\n",
        *_ms_struct.autobrew.preinf_on_time/10.,
        *_ms_struct.autobrew.preinf_off_time/10.,
        *_ms_struct.autobrew.timeout,
        *_ms_struct.autobrew.preinf_ramp_time/10.,
        *_ms_struct.brew.dose/10.,
        *_ms_struct.brew.yield/10.,
        *_ms_struct.brew.temp/10.,
        *_ms_struct.hot.temp/10.,
        *_ms_struct.steam.temp/10.,
        *_ms_struct.autobrew.preinf_power,
        *_ms_struct.brew.power,
        *_ms_struct.hot.power);
    return PICO_ERROR_NONE;
}