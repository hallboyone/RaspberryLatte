/**
 * @ingroup drivers
 * @{
 * 
 * \file machine_settings.c
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Machine Settings source
 * \version 0.1
 * \date 2022-11-12
 */

#include "drivers/ulka_pump.h"

#include "stdlib.h"
#include <stdio.h>

#define ULKA_PUMP_FLOW_FILTER_SPAN_MS 1028
#define ULKA_PUMP_FLOW_SAMPLE_RATE_MS 200

/** \brief Struct containing the fields for the actuation and measurement of a single Ulka pump */
typedef struct ulka_pump_s {
    phasecontrol driver;   /**< \brief Phase-control object responsible for switching pump's SSR. */
    flow_meter flow_ul_ms; /**< \brief Flow meter measuring the current flow rate through the pump in ul/ms. */
    bool locked;           /**< \brief Flag indicating if the pump is locked. */
    uint8_t power_percent; /**< \brief The current percent power applied to the pump. */
} ulka_pump_;

/**
 * \brief LUT mapping percent power to duty cycle. 
 * 
 * Since low duty cycles don't operate the pump, 1% power corresponds with about 50% duty cycle.
 */
static const uint8_t _percent_to_power_lut [] = {
      0,  60,  61,  61,  62,  63,  63,  64,  65,  65,  66,  67,  67,  68,  69,  69,  70,  
     71,  72,  72,  73,  74,  74,  75,  76,  76,  77,  78,  78,  79,  80,  80,  81,  82,  
     82,  83,  84,  84,  85,  86,  86,  87,  88,  88,  89,  90,  90,  91,  92,  92,  93,  
     94,  95,  95,  96,  97,  97,  98,  99,  99, 100, 101, 101, 102, 103, 103, 104, 105, 
    105, 106, 107, 107, 108, 109, 109, 110, 111, 111, 112, 113, 113, 114, 115, 115, 116, 
    117, 118, 118, 119, 120, 120, 121, 122, 122, 123, 124, 124, 125, 126, 126, 127};

ulka_pump ulka_pump_setup(uint8_t zerocross_pin, uint8_t out_pin, int32_t zerocross_shift_us, uint8_t zerocross_event){
    ulka_pump p = malloc(sizeof(ulka_pump_));
    p->locked = true;
    p->power_percent = 0;
    p->flow_ul_ms = NULL;
    phasecontrol_setup((&p->driver), zerocross_pin, out_pin, zerocross_shift_us, zerocross_event);
    return p;
}

int ulka_pump_setup_flow_meter(ulka_pump p, uint8_t pin_num, uint16_t ul_per_tick){
    if(p->flow_ul_ms != NULL) flow_meter_deinit(p->flow_ul_ms);

    p->flow_ul_ms = flow_meter_setup(pin_num, ul_per_tick, ULKA_PUMP_FLOW_FILTER_SPAN_MS, ULKA_PUMP_FLOW_SAMPLE_RATE_MS);
    return (p->flow_ul_ms == NULL ? PICO_ERROR_GENERIC : PICO_ERROR_NONE);
}

void ulka_pump_pwr_percent(ulka_pump p, uint8_t power_percent){
    if(!p->locked){
        p->power_percent = (power_percent > 100 ? 100 : power_percent);
        phasecontrol_set_duty_cycle(&(p->driver), _percent_to_power_lut[p->power_percent]);
    }
}

void ulka_pump_off(ulka_pump p){
    ulka_pump_pwr_percent(p, 0);
}

void ulka_pump_lock(ulka_pump p){
    p->locked = true;
    ulka_pump_pwr_percent(p, 0);
}

void ulka_pump_unlock(ulka_pump p){
    p->locked = false;
}

uint8_t ulka_pump_get_pwr(ulka_pump p){
    return p->power_percent;
}

int16_t ulka_pump_get_flow_ul_s(ulka_pump p){
    return (p->flow_ul_ms == NULL ? 0 : 1000.0*flow_meter_rate(p->flow_ul_ms));
}

int16_t ulka_pump_get_pressure_mbar(ulka_pump p){
    if(p->flow_ul_ms == NULL) return 0;
    int16_t pressure = 0;
    if(p->power_percent == 0) return 0;
    else if(p->power_percent < 10){
        pressure =  431.9*p->power_percent - 0.6476*ulka_pump_get_flow_ul_s(p);
    } else if(p->power_percent < 20){
        pressure = 2642.6 +  68.6*p->power_percent - 1.0042*ulka_pump_get_flow_ul_s(p);
    } else if(p->power_percent < 30){
        pressure = 4043.4 +  64.0*p->power_percent - 1.2913*ulka_pump_get_flow_ul_s(p);
    } else if(p->power_percent < 40){
        pressure = 2599.4 + 142.5*p->power_percent - 1.5014*ulka_pump_get_flow_ul_s(p);
    } else if(p->power_percent < 50){
        pressure = 2316.1 + 153.2*p->power_percent - 1.5692*ulka_pump_get_flow_ul_s(p);
    } else if(p->power_percent < 60){
        pressure = 1861.7 + 162.6*p->power_percent - 1.6878*ulka_pump_get_flow_ul_s(p);
    } else if(p->power_percent < 70){
        pressure = 5630.1 +  95.5*p->power_percent - 1.6701*ulka_pump_get_flow_ul_s(p);
    } else if(p->power_percent < 80){
        pressure = 6512.2 +  84.7*p->power_percent - 1.6412*ulka_pump_get_flow_ul_s(p);
    } else if(p->power_percent < 90){
        pressure = 2404.7 + 140.5*p->power_percent - 1.6984*ulka_pump_get_flow_ul_s(p);
    } else {
        pressure = 3.5282 + 0.1258*p->power_percent - 1.6838*ulka_pump_get_flow_ul_s(p);
    }
    return (pressure > 0 ? pressure : 0);
}

bool ulka_pump_is_locked(ulka_pump p){
    return p->locked;
}

void ulka_pump_deinit(ulka_pump p){
    if(p->flow_ul_ms != NULL) flow_meter_deinit(p->flow_ul_ms);
    free(p);
}
/** @} */