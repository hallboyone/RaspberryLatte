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
#include "utils/pid.h"
typedef struct {
    phasecontrol pump_driver;
    //pid_ctrl flow_controller;
    //pid_ctrl pressure_control;
    bool locked;
    float max_pressure;
} ulka_pump;

int ulka_pump_setup(ulka_pump * p, uint8_t zerocross_pin, uint8_t out_pin, int32_t zerocross_shift, uint8_t zerocross_event);

// int ulka_pump_setup_flow_control(ulka_pump * p);

// int ulka_pump_setup_pressure_control(ulka_pump * p);

int ulka_pump_pwr_percent(ulka_pump * p, uint8_t percent_power);

// int ulka_pump_pwr_flow(ulka_pump * p);

// int ulka_pump_pwr_pressure(ulka_pump * p);

int ulka_pump_off(ulka_pump * p);

int ulka_pump_lock(ulka_pump * p);

int ulka_pump_unlock(ulka_pump * p);

float ulka_pump_get_pwr(ulka_pump * p);

float ulka_pump_get_flow(ulka_pump * p);

float ulka_pump_get_pressure(ulka_pump * p);

bool ulka_pump_is_locked(ulka_pump * p);

#endif
/** @} */