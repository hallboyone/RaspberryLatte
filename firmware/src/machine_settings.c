#include "machine_settings.h"
#include "mb85_fram.h"

static const reg_addr MACHINE_SETTINGS_START_ADDR = 0x0000;

static const machine_settings MACHINE_SETTINGS_DEFAULT = {
    .power.brew_per  = 100,  // %100
    .power.pre_per   = 80,   // %80
    .temp.brew_dc    = 900,  // 90.0C
    .temp.hot_dc     = 1000, // 100.0C
    .temp.steam_dc   = 1450, // 145.0C
    .time.pre_off_ds = 40,   // 4.0s
    .time.pre_on_ds  = 40,   // 4.0s
    .time.ramp_ds    = 10,   // 1.0s
    .time.timeout_s  = 60,   // 60s
    .weight.dose_cg  = 1600, // 16.00g
    .weight.yield_cg = 3000  // 30.00g
};

static mb85_fram * _mem = NULL;
static machine_settings _settings;

bool machine_settings_verify(){
    if(
        _settings.power.brew_per < 50 || _settings.power.brew_per > 100  ||
        _settings.power.pre_per < 50  || _settings.power.pre_per > 100   ||
        _settings.temp.brew_dc < 500  || _settings.temp.brew_dc > 1450   ||
        _settings.temp.hot_dc < 500   || _settings.temp.hot_dc > 1450    ||
        _settings.temp.steam_dc < 500 || _settings.temp.steam_dc > 1450  ||
        _settings.time.pre_off_ds < 0 || _settings.time.pre_off_ds > 600 ||
        _settings.time.pre_on_ds < 0  || _settings.time.pre_on_ds > 600  ||
        _settings.time.ramp_ds < 0    || _settings.time.ramp_ds > 600    ||
        _settings.time.timeout_s < 25 || _settings.time.timeout_s > 120  ||
        _settings.weight.dose_cg < 0  || _settings.weight.dose_cg > 7500 ||
        _settings.weight.yield_cg < 0 || _settings.weight.yield_cg > 7500
    ){
        _settings = MACHINE_SETTINGS_DEFAULT;
        return true;
    } else {
        return false;
    }
}

static inline reg_addr machine_settings_id_to_addr(uint8_t id){
    return MACHINE_SETTINGS_START_ADDR + (1+id)*sizeof(machine_settings);
}

const machine_settings * machine_settings_setup(mb85_fram * mem){
    if(_mem == NULL){
        _mem = mem;
        if(mb85_fram_link_var(_mem, &_settings, MACHINE_SETTINGS_START_ADDR, sizeof(machine_settings), MB85_FRAM_INIT_FROM_FRAM)){
            return NULL;
        }
        if(machine_settings_verify()){
            mb85_fram_save(_mem, &_settings);
        }
    }
    return & _settings;
}

const machine_settings * machine_settings_aquire(){
    if(_mem == NULL) return NULL;
    else return &_settings;
}

int machine_settings_save_profile(uint8_t * profile_id){
    if(_mem == NULL) return PICO_ERROR_GENERIC;
    // Break link with profile buffer and connect to profile save address
    mb85_fram_unlink_var(_mem, &_settings);
    mb85_fram_link_var(_mem, &_settings, machine_settings_id_to_addr(profile_id), sizeof(machine_settings), MB85_FRAM_INIT_FROM_VAR);

    // Break link with save address and connect with profile buffer
    mb85_fram_unlink_var(_mem, &_settings);
    mb85_fram_link_var(_mem, &_settings, MACHINE_SETTINGS_START_ADDR, sizeof(machine_settings), MB85_FRAM_INIT_FROM_VAR);
    return PICO_ERROR_NONE;
}

int machine_settings_load_profile(uint8_t * profile_id){
    if(_mem == NULL) return PICO_ERROR_GENERIC;
    // Break link with profile buffer and connect to profile load address
    mb85_fram_unlink_var(_mem, &_settings);
    mb85_fram_link_var(_mem, &_settings, machine_settings_id_to_addr(profile_id), sizeof(machine_settings), MB85_FRAM_INIT_FROM_FRAM);

    // Verify that loaded address is valid. If it wasn't, save back into profile.
    if(machine_settings_verify()) mb85_fram_save(_mem, &_settings);

    // Break link with load address and connect with profile buffer
    mb85_fram_unlink_var(_mem, &_settings);
    mb85_fram_link_var(_mem, &_settings, MACHINE_SETTINGS_START_ADDR, sizeof(machine_settings), MB85_FRAM_INIT_FROM_VAR);
    return PICO_ERROR_NONE;
}

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