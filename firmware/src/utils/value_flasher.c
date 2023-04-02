/** @ingroup value_flasher
 * @{
 * 
 * \file value_flasher.c
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Value Flasher source
 * \version 0.1
 * \date 2022-11-12 
 */

#include "utils/value_flasher.h"

#include <stdlib.h>

#include "utils/macros.h"

/** \brief A single value flasher that will display a single value. */
typedef struct value_flasher_s {
    uint16_t value_to_output; /**< The value that will be flashed */
    uint8_t * out_flags;      /**< Pointer to the output bitfield */
    int64_t period_us;        /**< The period of one blink */
    uint16_t cur_val;        /**< Internal variable tracking progress through blink sequence */
    alarm_id_t alarm_id;     /**< The ID of the alarm that increments the blink sequence */
} value_flasher_;

/**
 * \brief Callback for value_flasher alarms. If the alarm_id has not been changed (signaling
 * the end of the flasher), the blink sequence is incremented.
 * 
 * \param id The ID of the alarm
 * \param user_data Pointer to the corresponding value_flasher
 * \return int64_t microseconds to next callback or 0 if value_flasher has ended.
 */
static int64_t _value_flasher_callback(alarm_id_t id, void *user_data){
    UNUSED(id);
    
    value_flasher vf = (value_flasher)user_data;
    
    if(vf->alarm_id == -1) return 0;

    if (*(vf->out_flags) != 0){
        *(vf->out_flags) = 0;
    } else {
        if (vf->cur_val >= 100){
            *(vf->out_flags) = 0b100;
            vf->cur_val -= 100;
        } else if (vf->cur_val >= 10){
            *(vf->out_flags) = 0b010;
            vf->cur_val -= 10;
        } else if (vf->cur_val >= 1){
            *(vf->out_flags) = 0b001;
            vf->cur_val -= 1;
        } else {
            vf->cur_val = vf->value_to_output;
        }
    }
    
    return vf->period_us;
}

value_flasher value_flasher_setup(uint16_t value, uint16_t period_ms, uint8_t * bitfield){
    value_flasher vf = malloc(sizeof(value_flasher_));

    vf->cur_val = value;
    vf->value_to_output = value;
    vf->out_flags = bitfield;
    *(vf->out_flags) = 0b000;
    vf->period_us = period_ms;
    vf->period_us *= 1000;
    vf->alarm_id = -1;

    return vf;
}

void value_flasher_update(value_flasher vf, uint16_t value){
    vf->cur_val = value;
    vf->value_to_output = value;
    *(vf->out_flags) = 0b000;
}

void value_flasher_start(value_flasher vf){
    value_flasher_end(vf);
    vf->cur_val = vf->value_to_output;
    *(vf->out_flags) = 0b000;
    vf->alarm_id = add_alarm_in_us(vf->period_us, &_value_flasher_callback, vf, true);
}

void value_flasher_end(value_flasher vf){
    if(vf->alarm_id != -1){
        cancel_alarm(vf->alarm_id);
    }
    vf->alarm_id = -1;
}

void value_flasher_deinit(value_flasher vf){
    value_flasher_end(vf);
    free(vf);
}
/** @} */