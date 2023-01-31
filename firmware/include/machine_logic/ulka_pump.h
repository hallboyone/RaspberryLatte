/**
 * \defgroup ulka_pump Ulka Pump Library
 * \version 0.1
 * 
 * \brief Abstracts the use and control of Ulka vibratory pump
 * 
 * \ingroup machine_logic
 * @{
 * \file
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Header for Ulka Pump Library
 * \version 0.1
 * \date 2023-01-30
 */

#ifndef ULKA_PUMP_H
#define ULKA_PUMP_H
#include "pico/stdlib.h"
#include "utils/phasecontrol.h"

typedef struct {
    phasecontrol pump_driver;
} ulka_pump_t;

#endif
/** @} */