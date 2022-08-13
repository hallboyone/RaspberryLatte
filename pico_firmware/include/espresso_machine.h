#ifndef _ESPRESSO_MACHINE_H
#define _ESPRESSO_MACHINE_H
#include "pico/stdlib.h"

enum espresso_machine_modes {MODE_STEAM, MODE_HOT, MODE_MANUAL, MODE_AUTO};

typedef struct {
    bool ac_switch;
    bool pump_switch;
    uint8_t mode_dial;
} espresso_machine_switches;

typedef struct {
    uint16_t setpoint;
    int16_t tempurature;
    uint8_t power_level;
    uint16_t error_sum;
    uint16_t error_slope;
} espresso_machine_boiler;

typedef struct {
    uint8_t power_level;
    bool pump_lock;
    int8_t brew_leg;
} espresso_machine_pump;

typedef struct {
    int32_t val_mg;
} espresso_machine_scale;

typedef struct {
    espresso_machine_switches switches;
    espresso_machine_boiler boiler;
    espresso_machine_pump pump;
    espresso_machine_scale scale;
} espresso_machine_state;

typedef const espresso_machine_state * espresso_machine_viewer;

int espresso_machine_setup(espresso_machine_viewer * state_viewer);
void espresso_machine_tick();

#endif