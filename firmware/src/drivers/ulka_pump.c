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

#define ULKA_PUMP_FLOW_FILTER_SPAN_MS 2005
#define ULKA_PUMP_FLOW_SAMPLE_RATE_MS 100

/** \brief Struct containing the fields for the actuation and measurement of a single Ulka pump */
typedef struct ulka_pump_s {
    phasecontrol driver;   /**< \brief Phase-control object responsible for switching pump's SSR. */
    flow_meter flow_ml_s; /**< \brief Flow meter measuring the current flow rate through the pump in ul/ms. */
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

#define NUM_LINEAR_REGIONS 10 // should be divisor of 100 and match linear coefficients arrays below
static const uint8_t LINEAR_REGION_SPAN = 100/NUM_LINEAR_REGIONS;
static const float OFFSET [NUM_LINEAR_REGIONS]    = { 0,      2.6426, 4.0434, 2.5994, 2.3161, 1.8617, 5.6301, 6.5122, 2.4047, 3.5282};
static const float PUMP_GAIN [NUM_LINEAR_REGIONS] = { 0.4319, 0.0686, 0.0640, 0.1425, 0.1532, 0.1626, 0.0955, 0.0847, 0.1405, 0.1258};
static const float FLOW_GAIN [NUM_LINEAR_REGIONS] = {-0.6476,-1.0042,-1.2913,-1.5014,-1.5692,-1.6878,-1.6701,-1.6412,-1.6984,-1.6838};

/**
 * \brief Returns the pump power needed to maintain the target pressure.
 * \param p The ulka_pump that should be regulated.
 * \param target_pressure The pressure that is being targeted
 * \returns The pump power required to reach the target pressure, clipped between 0 and 100. 
*/
static uint8_t ulka_pump_power_to_get_pressure(ulka_pump p, const float target_pressure){
    if(p->flow_ml_s == NULL || target_pressure < 0) return 0;
    
    const float flowrate = ulka_pump_get_flow_ml_s(p);
    for(uint i = 0; i < NUM_LINEAR_REGIONS; i++){
        // Check if the power in each linear region is strong enough to reach flowrate. If it is, then compute
        // the required power and return.
        if(OFFSET[i] + PUMP_GAIN[i]*LINEAR_REGION_SPAN*(i+1) + FLOW_GAIN[i]*flowrate > target_pressure){
            float power = (target_pressure - FLOW_GAIN[i]*flowrate - OFFSET[i])/PUMP_GAIN[i];
            return (power < 0 ? 0 : (uint8_t)power);
        }
    }
    // None of the regions are strong enough. Return full strength.
    return 100;
}

ulka_pump ulka_pump_setup(uint8_t zerocross_pin, uint8_t out_pin, int32_t zerocross_shift_us, uint8_t zerocross_event){
    ulka_pump p = malloc(sizeof(ulka_pump_));
    p->locked = true;
    p->power_percent = 0;
    p->flow_ml_s = NULL;
    p->driver = phasecontrol_setup(zerocross_pin, out_pin, zerocross_shift_us, zerocross_event);
    return p;
}

int ulka_pump_setup_flow_meter(ulka_pump p, uint8_t pin_num, float ml_per_tick){
    if(p->flow_ml_s != NULL) flow_meter_deinit(p->flow_ml_s);

    p->flow_ml_s = flow_meter_setup(pin_num, ml_per_tick, ULKA_PUMP_FLOW_FILTER_SPAN_MS, ULKA_PUMP_FLOW_SAMPLE_RATE_MS);
    return (p->flow_ml_s == NULL ? PICO_ERROR_GENERIC : PICO_ERROR_NONE);
}

void ulka_pump_pwr_percent(ulka_pump p, uint8_t power_percent){
    if(!p->locked){
        const uint8_t max_power = ulka_pump_power_to_get_pressure(p, 9.5);
        p->power_percent = (power_percent > max_power ? max_power : power_percent);
        phasecontrol_set_duty_cycle(p->driver, _percent_to_power_lut[p->power_percent]);
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

float ulka_pump_get_flow_ml_s(ulka_pump p){
    return (p->flow_ml_s == NULL ? 0 : flow_meter_rate(p->flow_ml_s));
}

float ulka_pump_get_pressure_bar(ulka_pump p){
    if(p->flow_ml_s == NULL) return 0;

    // Index of the linear region indexed by the percent power
    const uint8_t active_region = p->power_percent/LINEAR_REGION_SPAN;
    const float pressure =   OFFSET[active_region] 
                     + PUMP_GAIN[active_region]*p->power_percent 
                     + FLOW_GAIN[active_region]*ulka_pump_get_flow_ml_s(p);
    return (pressure > 0 ? pressure : 0);
}

bool ulka_pump_is_locked(ulka_pump p){
    return p->locked;
}

void ulka_pump_deinit(ulka_pump p){
    if(p->flow_ml_s != NULL) flow_meter_deinit(p->flow_ml_s);
    phasecontrol_deinit(p->driver);
    free(p);
}
/** @} */