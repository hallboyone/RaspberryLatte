#ifndef MACHINE_PARAMETERS_H
#define MACHINE_PARAMETERS_H

#define MACHINE_PARAMETERS_STARTING_ADDRESS 0x00

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

typedef struct {
    uint8_t pre_on_ds;
    uint8_t pre_off_ds;
    uint8_t timeout_s;
    uint8_t ramp_ds;
} brew_times;

typedef struct {
    uint16_t dose_cg;
    uint16_t yield_cg;
    uint16_t brew_temp_cC;
    brew_times times;
} brew_profile;

typedef struct {
    uint8_t active_profile;
    brew_profile profile [9];
    
} machine_parameters;

#endif