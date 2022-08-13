#ifndef _LMT01_H
#define _LMT01_H

#include "pico/stdlib.h"
#include "hardware/pio.h"

typedef struct{
    PIO _pio;
    uint _sm;
    uint8_t _dat_pin;
    int _latest_temp;
} lmt01;

/**
 * \brief Configures the signal pin attached to a LMT01 temp sensor and starts a PIO 
 * program that counts the sensors pulse train
 * 
 * \param pio_num Either 0 or 1 indicating if PIO #0 or #1 should be used
 * \param dat_pin Pin that the LMT01 is attached to
 */
void lmt01_setup(lmt01 * l, uint8_t pio_num, uint8_t dat_pin);

/**
 * \brief Returns the current temputature in 16*C. Divide by 16 6o conver to C
 */
int lmt01_read(lmt01 * l);

/**
 * \brief Returns the current tempurature in C.
 */
float lmt01_read_float(lmt01 * l);
#endif