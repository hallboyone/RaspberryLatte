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

static const uint8_t _percent_to_power_lut [] = {
      0,  60,  61,  61,  62,  63,  63,  64,  65,  65,  66,  67,  67,  68,  69,  69,  70,  
     71,  72,  72,  73,  74,  74,  75,  76,  76,  77,  78,  78,  79,  80,  80,  81,  82,  
     82,  83,  84,  84,  85,  86,  86,  87,  88,  88,  89,  90,  90,  91,  92,  92,  93,  
     94,  95,  95,  96,  97,  97,  98,  99,  99, 100, 101, 101, 102, 103, 103, 104, 105, 
    105, 106, 107, 107, 108, 109, 109, 110, 111, 111, 112, 113, 113, 114, 115, 115, 116, 
    117, 118, 118, 119, 120, 120, 121, 122, 122, 123, 124, 124, 125, 126, 126, 127};

int ulka_pump_setup(ulka_pump * p, uint8_t zerocross_pin, uint8_t out_pin, int32_t zerocross_shift_us, uint8_t zerocross_event){
    p->locked = true;
    p->percent_power = 0;
    phasecontrol_setup((&p->pump_driver), zerocross_pin, out_pin, zerocross_shift_us, zerocross_event);
}

int ulka_pump_setup_flow_meter(ulka_pump * p, uint8_t pin_num, float conversion){
    return flow_meter_setup(&(p->pump_flow_meter), pin_num, conversion);
}

void ulka_pump_pwr_percent(ulka_pump * p, uint8_t percent_power){
    if(!p->locked){
        p->percent_power = (percent_power > 100 ? 100 : percent_power);
        phasecontrol_set_duty_cycle(&(p->pump_driver), _percent_to_power_lut[p->percent_power]);
    }
}

void ulka_pump_off(ulka_pump * p){
    ulka_pump_pwr_percent(&(p->pump_driver), 0);
}

void ulka_pump_lock(ulka_pump * p){
    p->locked = true;
    ulka_pump_pwr_percent(&(p->pump_driver), 0);
}

void ulka_pump_unlock(ulka_pump * p){
    p->locked = false;
}

uint8_t ulka_pump_get_pwr(ulka_pump * p){
    return p->percent_power;
}

float ulka_pump_get_flow(ulka_pump * p){
    return flow_meter_rate(&(p->pump_flow_meter));
}

float ulka_pump_get_pressure(ulka_pump * p){
    return 0.2*p->percent_power - 1.4757*ulka_pump_get_flow(p) - 9.2;
}

bool ulka_pump_is_locked(ulka_pump * p){
    return p->locked;
}
/** @} */