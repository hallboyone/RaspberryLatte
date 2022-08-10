#ifndef _AUTOBREW_H
#define _AUTOBREW_H
#include "pico/stdlib.h"

typedef enum {FUNCTION_CALL, RAMP, CONSTANT_TIMED, CONSTANT_TRIGGERED} leg_type_names;

typedef void (*autobrew_fun)();
typedef bool (*autobrew_trigger)();

typedef struct {
    leg_type_names leg_type;

    uint8_t pump_pwr_start; // Starting or constant power
    uint8_t pump_pwr_delta; // Change in power over leg

    uint32_t duration_us;  // Duration (or timeout) of leg in ms
    uint64_t end_time_us;  // End time of leg

    autobrew_fun fun;   // Void function to call if FUNCTION_CALL leg
    autobrew_trigger trigger; // Function returning bool triggering some action
} autobrew_leg;

typedef struct {
    uint8_t pump_setting;
    bool pump_setting_changed;
    bool finished_leg;
} autobrew_state;

typedef struct {
    uint8_t num_legs;
    uint8_t cur_leg;
    autobrew_leg * legs;
} autobrew_routine;

int autobrew_setup_function_call_leg(autobrew_leg * leg, autobrew_fun fun, uint8_t pwr);

int autobrew_setup_ramp_leg(autobrew_leg * leg, uint8_t pump_starting_pwr, 
                            uint8_t pump_ending_power, uint32_t duration_us);

int autobrew_setup_constant_timed_leg(autobrew_leg * leg, uint8_t pump_pwr, 
                                      uint32_t duration_us);

int autobrew_setup_constant_triggered_leg(autobrew_leg * leg, uint8_t pump_pwr, 
                                          autobrew_trigger trigger, uint32_t timeout_us);

int autobrew_setup_routine(autobrew_routine * r, autobrew_leg * legs, uint8_t num_legs);

#endif