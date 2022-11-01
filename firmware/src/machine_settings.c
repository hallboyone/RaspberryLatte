#include "machine_settings.h"
#include <stdio.h>
#include <string.h>

static const reg_addr MACHINE_SETTINGS_START_ADDR = 0x0000;
static const uint16_t MACHINE_SETTINGS_MEMORY_SIZE = MS_COUNT*sizeof(machine_setting);

typedef struct{
    machine_setting min;
    machine_setting nomn;
    machine_setting max;
} machine_setting_limits;

static const machine_setting _setting_min [MS_COUNT] = {
    0,  // MACHINE_SETTING_TIME_PREINF_ON_DS
    0,  // MACHINE_SETTING_TIME_PRE_OFF_DS
    20, // MACHINE_SETTING_TIME_TIMEOUT_S
    0,  // MACHINE_SETTING_TIME_RAMP_DS
    0,  // MACHINE_SETTING_WEIGHT_DOSE_CG
    0,  // MACHINE_SETTING_WEIGHT_YIELD_CG
    0,  // MACHINE_SETTING_TEMP_BREW_DC
    0,  // MACHINE_SETTING_TEMP_HOT_DC
    0,  // MACHINE_SETTING_TEMP_STEAM_DC
    60, // MACHINE_SETTING_PWR_PREINF_PER
    60  // MACHINE_SETTING_PWR_BREW_PER
};

static const machine_setting _setting_default [MS_COUNT] = {
    40,   // MACHINE_SETTING_TIME_PREINF_ON_DS
    40,   // MACHINE_SETTING_TIME_PRE_OFF_DS
    60,   // MACHINE_SETTING_TIME_TIMEOUT_S
    10,   // MACHINE_SETTING_TIME_RAMP_DS
    1500, // MACHINE_SETTING_WEIGHT_DOSE_CG
    3000, // MACHINE_SETTING_WEIGHT_YIELD_CG
    900,  // MACHINE_SETTING_TEMP_BREW_DC
    1000, // MACHINE_SETTING_TEMP_HOT_DC
    1450, // MACHINE_SETTING_TEMP_STEAM_DC
    80,   // MACHINE_SETTING_PWR_PREINF_I8
    127   // MACHINE_SETTING_PWR_BREW_I8
};

static const machine_setting _setting_max [MS_COUNT] = {
    600,  // MACHINE_SETTING_TIME_PREINF_ON_DS
    600,  // MACHINE_SETTING_TIME_PRE_OFF_DS
    180,  // MACHINE_SETTING_TIME_TIMEOUT_S
    600,  // MACHINE_SETTING_TIME_RAMP_DS
    3000, // MACHINE_SETTING_WEIGHT_DOSE_CG
    6000, // MACHINE_SETTING_WEIGHT_YIELD_CG
    1450, // MACHINE_SETTING_TEMP_BREW_DC
    1450, // MACHINE_SETTING_TEMP_HOT_DC
    1450, // MACHINE_SETTING_TEMP_STEAM_DC
    127,  // MACHINE_SETTING_PWR_PREINF_I8
    127   // MACHINE_SETTING_PWR_BREW_I8
};

static mb85_fram * _mem = NULL;
static machine_setting _settings [MS_COUNT];

bool machine_settings_verify(){
    for(uint8_t p_id = 0; p_id < MS_COUNT; p_id++){
        if(_settings[p_id] < _setting_min[p_id] || _settings[p_id] > _setting_max[p_id]){
            memcpy(_settings, _setting_default, MACHINE_SETTINGS_MEMORY_SIZE);
            return true;
        }
    }
    return false;
}

static inline reg_addr machine_settings_id_to_addr(uint8_t id){
    return MACHINE_SETTINGS_START_ADDR + (1+id)*MACHINE_SETTINGS_MEMORY_SIZE;
}

machine_settings machine_settings_setup(mb85_fram * mem){
    if(_mem == NULL){
        _mem = mem;
        if(mb85_fram_link_var(_mem, &_settings, MACHINE_SETTINGS_START_ADDR, MACHINE_SETTINGS_MEMORY_SIZE, MB85_FRAM_INIT_FROM_FRAM)){
            return NULL;
        }
        if(machine_settings_verify()){
            mb85_fram_save(_mem, &_settings);
        }
    }
    return _settings;
}

machine_settings machine_settings_aquire(){
    if(_mem == NULL) return NULL;
    else return _settings;
}

int machine_settings_save_profile(uint8_t profile_id){
    if(_mem == NULL) return PICO_ERROR_GENERIC;
    // Break link with profile buffer and connect to profile save address
    mb85_fram_unlink_var(_mem, &_settings);
    mb85_fram_link_var(_mem, &_settings, machine_settings_id_to_addr(profile_id), MACHINE_SETTINGS_MEMORY_SIZE, MB85_FRAM_INIT_FROM_VAR);

    // Break link with save address and connect with profile buffer
    mb85_fram_unlink_var(_mem, &_settings);
    mb85_fram_link_var(_mem, &_settings, MACHINE_SETTINGS_START_ADDR, MACHINE_SETTINGS_MEMORY_SIZE, MB85_FRAM_INIT_FROM_VAR);
    return PICO_ERROR_NONE;
}

int machine_settings_load_profile(uint8_t profile_id){
    if(_mem == NULL) return PICO_ERROR_GENERIC;
    // Break link with profile buffer and connect to profile load address
    mb85_fram_unlink_var(_mem, &_settings);
    mb85_fram_link_var(_mem, &_settings, machine_settings_id_to_addr(profile_id), MACHINE_SETTINGS_MEMORY_SIZE, MB85_FRAM_INIT_FROM_FRAM);

    // Verify that loaded address is valid. If it wasn't, save back into profile.
    if(machine_settings_verify()) mb85_fram_save(_mem, &_settings);

    // Break link with load address and connect with profile buffer
    mb85_fram_unlink_var(_mem, &_settings);
    mb85_fram_link_var(_mem, &_settings, MACHINE_SETTINGS_START_ADDR, MACHINE_SETTINGS_MEMORY_SIZE, MB85_FRAM_INIT_FROM_VAR);
    return PICO_ERROR_NONE;
}

int machine_settings_set(machine_setting_id p_id, machine_setting val){
    if(p_id >= MS_COUNT) return PICO_ERROR_INVALID_ARG;
    
    if(val < _setting_min[p_id]){
        val = _setting_min[p_id];
    } else if (val > _setting_max[p_id]){
        val = _setting_max[p_id];
    }
    _settings[p_id] = val;
    return PICO_ERROR_NONE;
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
        "Brew power         : %d\%\n",
        _settings[0]/10.,
        _settings[1]/10.,
        _settings[2],
        _settings[3]/10.,
        _settings[4]/100.,
        _settings[5]/100.,
        _settings[6]/10.,
        _settings[7]/10.,
        _settings[8]/10.,
        _settings[9],
        _settings[10]);
}