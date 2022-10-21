#ifndef VALUE_FLASHER_H
#define VALUE_FLASHER_H

#include "pico/stdlib.h"

typedef struct {
    uint16_t value_to_output;
    uint8_t out_flags;

    uint16_t cur_val;
    int64_t period_us;
    alarm_id_t alarm_id;
} value_flasher;

void value_flasher_setup(value_flasher * vf, uint16_t value, uint16_t period_ms);
void value_flasher_update(value_flasher * vf, uint16_t value);
void value_flasher_end(value_flasher * vf);
#endif