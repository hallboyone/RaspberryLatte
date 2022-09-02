//#define COUNTER_CULTURE_SLOW_MOTION
//#define COUNTER_CULTURE_FORTY_SIX
#define CAMERONS_HAWAIIAN_BLEND

#ifdef COUNTER_CULTURE_FORTY_SIX
const float TEMP_SETPOINTS [4] = {140, 100, 93, 93};
#endif

#ifdef COUNTER_CULTURE_SLOW_MOTION
const float TEMP_SETPOINTS [4] = {140, 100, 95, 95};
#endif

#ifdef CAMERONS_HAWAIIAN_BLEND
const float TEMP_SETPOINTS [4] = {140, 100, 95, 95};
#endif

const float PID_GAIN_P = 0.05;
const float PID_GAIN_I = 0.0015;
const float PID_GAIN_D = 0.0005;

const float SCALE_CONVERSION_MG = 0.152710615479;

const unsigned int BREW_DOSE_MG = 16000;
const unsigned int BREW_YIELD_MG = 30000;
const unsigned int AUTOBREW_PREINFUSE_END_POWER    = 80;
const unsigned int AUTOBREW_PREINFUSE_ON_TIME_US   = 4000000;
const unsigned int AUTOBREW_PREINFUSE_OFF_TIME_US  = 4000000;
const unsigned int AUTOBREW_BREW_RAMP_TIME         = 1000000;   
const unsigned int AUTOBREW_BREW_TIMEOUT_US        = 60000000; // 60s
