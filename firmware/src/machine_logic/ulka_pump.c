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

#include "machine_logic/ulka_pump.h"

/**
 * \brief Convert percent power to value between 60 and 127 or 0 if percent is 0.
 * 
 * \param percent An integer percentage between 0 and 100
 * \return 0 if percent is 0. Else a value between 60 and 127
*/
static inline uint8_t _convert_pump_power(uint8_t percent){
    return (percent==0) ? 0 : 0.6 * percent + 67;
}

/** @} */