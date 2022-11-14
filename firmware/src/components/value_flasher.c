/** @ingroup value_flasher
 * @{
 * 
 * \file value_flasher.c
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Value Flasher source
 * \version 0.1
 * \date 2022-11-12 
 */

#include "value_flasher.h"

/**
 * \brief Callback for value_flasher alarms. If the alarm_id has not been changed (signaling
 * the end of the flasher), the blink sequence is incremented.
 * 
 * \param id The ID of the alarm
 * \param user_data Pointer to the corresponding value_flasher
 * \return int64_t microseconds to next callback or 0 if value_flasher has ended.
 */
static int64_t _value_flasher_callback(alarm_id_t id, void *user_data){
    value_flasher * vf = (value_flasher*)user_data;
    
    if(vf->_alarm_id != id) return 0;

    if (vf->out_flags != 0){
        *(vf->out_flags) = 0;
    } else {
        if (vf->_cur_val >= 100){
            *(vf->out_flags) = 0b100;
            vf->_cur_val -= 100;
        } else if (vf->_cur_val >= 10){
            *(vf->out_flags) = 0b010;
            vf->_cur_val -= 10;
        } else if (vf->_cur_val >= 1){
            *(vf->out_flags) = 0b001;
            vf->_cur_val -= 1;
        } else {
            vf->_cur_val = vf->value_to_output;
        }
    }
    
    return vf->period_us;
}

void value_flasher_setup(value_flasher * vf, uint16_t value, uint16_t period_ms, uint8_t * bitfield){
    vf->_cur_val = value;
    vf->value_to_output = value;
    vf->out_flags = bitfield;
    *(vf->out_flags) = 0b000;
    vf->period_us = period_ms;
    vf->period_us *= 1000;
    vf->_alarm_id = add_alarm_in_us(vf->period_us, &_value_flasher_callback, vf, true);
}

void value_flasher_update(value_flasher * vf, uint16_t value){
    vf->_cur_val = value;
    vf->value_to_output = value;
    *(vf->out_flags) = 0b000;
}


void value_flasher_end(value_flasher * vf){
    // By changing the alarm_id, the next trigger will simply end the alarm.
    vf->_alarm_id--;
}
/** @} */