#include "value_flasher.h"


static int64_t value_flasher_callback(alarm_id_t id, void *user_data){
    value_flasher * vf = (value_flasher*)user_data;
    
    if(vf->alarm_id != id) return 0;

    if (vf->out_flags != 0){
        vf->out_flags = 0;
    } else {
        if (vf->cur_val >= 100){
            vf->out_flags = 0b100;
            vf->cur_val -= 100;
        } else if (vf->cur_val >= 10){
            vf->out_flags = 0b010;
            vf->cur_val -= 10;
        } else if (vf->cur_val >= 1){
            vf->out_flags = 0b001;
            vf->cur_val -= 1;
        } else {
            vf->cur_val = vf->value_to_output;
        }
    }
    
    return vf->period_us;
}

void value_flasher_setup(value_flasher * vf, uint16_t value, uint16_t period_ms){
    vf->cur_val = value;
    vf->value_to_output = value;
    vf->out_flags = 0b000;
    vf->period_us = period_ms;
    vf->period_us *= 1000;
    vf->alarm_id = add_alarm_in_us(vf->period_us, &value_flasher_callback, vf, true);
}

void value_flasher_update(value_flasher * vf, uint16_t value){
    vf->cur_val = value;
    vf->value_to_output = value;
    vf->out_flags = 0b000;
}


void value_flasher_end(value_flasher * vf){
    cancel_alarm(vf->alarm_id);
}