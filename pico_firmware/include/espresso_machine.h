#ifndef _ESPRESSO_MACHINE_H
#define _ESPRESSO_MACHINE_H
#include "pico/stdlib.h"

enum espresso_machine_modes {MODE_STEAM, MODE_HOT, MODE_MANUAL, MODE_AUTO};

int espresso_machine_setup();
void espresso_machine_tick();

#endif