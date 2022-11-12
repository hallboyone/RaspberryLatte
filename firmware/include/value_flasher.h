/**
 * \file value_flasher.h
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Displays an integer value up to 999 by flashing bits in a bitfield.
 * \version 0.1
 * \date 2022-11-12 
 * 
 * The value_flasher object will point at an external bitfield with bits 0 representing the
 * ones place, bit 1 the tens, and bit 2 the hundreds. Once setup, the hundreds bit will
 * blink i_100 times at a user set frequency where i_100 is the number in the value's 
 * hundred's place. This continues with the tens and ones bits and then the process will
 * repeat. These bits can be displayed with the binary_output library, for example, to blink
 * 3 LEDs to output the value.
 */
#ifndef VALUE_FLASHER_H
#define VALUE_FLASHER_H

#include "pico/stdlib.h"

/** \brief A single value flasher that will display a single value. */
typedef struct {
    uint16_t value_to_output; /**< The value that will be flashed */
    uint8_t * out_flags;      /**< Pointer to the output bitfield */
    int64_t period_us;        /**< The period of one blink */
    uint16_t _cur_val;        /**< Internal variable tracking progress through blink sequence */
    alarm_id_t _alarm_id;     /**< The ID of the alarm that increments the blink sequence */
} value_flasher;

/**
 * \brief Setup a single value_flasher object to display \p value.
 * 
 * \param vf Pointer to value_flasher struct
 * \param value The value that will be displayed
 * \param period_ms The period of the blinks
 * \param bitfield Pointer to the external bitfield that will be blinked 
 */
void value_flasher_setup(value_flasher * vf, uint16_t value, uint16_t period_ms, uint8_t * bitfield);

/**
 * \brief Change the value that \p vf is displaying.
 * 
 * \param vf Pointer to previously setup value_flasher struct
 * \param value New value to display
 */
void value_flasher_update(value_flasher * vf, uint16_t value);

/**
 * \brief Stop displaying the value. Alarm will end at next blink and external bitfield
 * will not be updated.
 * 
 * \param vf Pointer to value_flasher that should be stopped. 
 */
void value_flasher_end(value_flasher * vf);
#endif