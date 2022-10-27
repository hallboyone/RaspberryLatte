#include "machine_parameters.h"
#include "mb85_fram.h"

static mb85_fram * _mem;
const reg_addr ACTIVE_PROFILE   = 0x0000;
const reg_addr MACHINE_PROFILES = 0x0001;

static uint8_t _active_profile_id;
static machine_parameters _machine_profiles [9];

bool machine_parameters_make_valid(machine_parameters * p);

int machine_parameters_setup(machine_parameters ** p, mb85_fram * mem){
    _mem = mem;
    mb85_fram_link_var(_mem, &_active_profile_id, ACTIVE_PROFILE, 1, MB85_FRAM_INIT_FROM_FRAM);
    if(_active_profile_id > 8){
        _active_profile_id = 0;
        mb85_fram_save(_mem, &_active_profile_id);
    }

    for (uint8_t i = 0; i<9; i++){
        mb85_fram_link_var(_mem, &_machine_profiles[i], MACHINE_PROFILES + i*sizeof(machine_parameters), 
                           sizeof(machine_parameters), MB85_FRAM_INIT_FROM_FRAM);
        if (machine_parameters_make_valid(&_machine_profiles[i])){
            mb85_fram_save(_mem, &_machine_profiles[i]);
        }
    }

    *p = _machine_profiles[_active_profile_id];
}

int machine_parameters_save(machine_parameters * p);