/**
 * \ingroup binary_input
 * 
 * \file binary_input.c
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief GPIO Binary-input Library source
 * \version 0.2
 * \date 2022-08-16
*/

#include "utils/binary_input.h"

#include <stdlib.h>
#include <string.h>

#include "utils/gpio_irq_timestamp.h"

/**
 * \brief Check if binary input is bouncing.
 * 
 * The input is bouncing if any of its pins have changed within the last debounce_us
 * microseconds. First, if debounce_us==0 then the input is never bouncing. Then, iterate
 * through all pins and see if any have recently changed. If yes, then input is bouncing.
 * If not, however, then input is not bouncing
 * 
 * \param b Configured binary input
 * \return True if input is bouncing. False if it is not.
 */
static bool _binary_input_bouncing(binary_input * b){
    if(b->debounce_us == 0) return false;
    for(uint8_t p_idx = 0; p_idx < b->num_pins; p_idx++){
        const uint8_t p = b->pins[p_idx];
        if(gpio_irq_timestamp_read_duration_us(p) < b->debounce_us) return true;
    }
    return false;
}

/**
 * \brief If input is not bouncing, iterates through each pin and saves its state 
 * while taking into account pull direction and inversion.
 * 
 * \param b Pointer to binary_input object that will be updated.
 */
static inline void _binary_input_update_pin_states(binary_input * b){
    if (!_binary_input_bouncing(b)){
        for(uint8_t p_idx = 0; p_idx < b->num_pins; p_idx++){
            uint8_t p = b->pins[p_idx];
            bool var = (gpio_is_pulled_down(p) ? gpio_get(p) : !gpio_get(p));
            b->pin_states[p_idx] = b->inverted ? !var : var;
        }
    }
}

void binary_input_setup(binary_input * b, uint8_t num_pins, const uint8_t * pins, binary_input_pull_dir pull_dir, uint debounce_us, bool invert, bool muxed){
    // Copy data into b.
    b->num_pins    = num_pins;
    b->pins        = (uint8_t*)malloc(sizeof(uint8_t) * num_pins);
    b->pin_states  = (bool*)malloc(sizeof(bool) * num_pins);
    b->debounce_us = debounce_us;
    b->muxed       = muxed;
    b->inverted    = invert;

    // Init the allocated arrays
    memcpy(b->pins, pins, num_pins);
    for(uint i = 0; i < num_pins; i++) b->pin_states[i] = false;

    // Setup each pin.
    for (uint8_t p = 0; p < b->num_pins; p++) {
        gpio_init(b->pins[p]);
        gpio_set_dir(b->pins[p], false);
        gpio_set_pulls(b->pins[p], pull_dir == BINARY_INPUT_PULL_UP,
                       pull_dir != BINARY_INPUT_PULL_UP);
        if(debounce_us > 0){
            // Attach irq to each pin if debouncing
            gpio_irq_timestamp_setup(b->pins[p], GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE);
        }
    }
}

int binary_input_read(binary_input * b) {
    _binary_input_update_pin_states(b);
    if (b->muxed) {
        uint8_t pin_mask = 0;
        for (uint8_t n = 0; n < b->num_pins; n++) {
            pin_mask = pin_mask | (b->pin_states[n] << n);
        }
        return pin_mask;
    } else {
        for (uint8_t n = 0; n < b->num_pins; n++) {
            if(b->pin_states[n]){
                return n+1;
            }
        }
        return 0;
    }
}