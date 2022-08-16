#include <stdlib.h>
#include "binary_input.h"

#include <string.h>

#include "status_ids.h"

/**
 * \brief Reads the indicated GPIO pin and inverts the results if the pin is pulled up. Inverts
 * again if invert is true.
 * 
 * \param pin_idx GPIO number to read
 * \param invert Inverts the result if true.
 * 
 * \returns !invert if pin reads opposite its pull. Otherwise returns invert.
 */
static inline uint8_t gpio_get_w_pull_and_invert(uint pin_idx, bool invert) {
    bool var = (gpio_is_pulled_down(pin_idx) ? gpio_get(pin_idx) : !gpio_get(pin_idx));
    return invert ? !var : var;
}

void binary_input_setup(binary_input * b, uint8_t num_pins, const uint8_t * pins, uint8_t pull_dir, bool invert, bool muxed){
    // Copy data into b.
    b->num_pins = num_pins;
    b->pins = (uint8_t*)malloc(sizeof(uint8_t) * num_pins);
    memcpy(b->pins, pins, num_pins);
    b->muxed = muxed;
    b->inverted = invert;

    // Setup each pin.
    for (uint8_t p = 0; p < b->num_pins; p++) {
        gpio_init(b->pins[p]);
        gpio_set_dir(b->pins[p], false);
        gpio_set_pulls(b->pins[p], pull_dir == BINARY_INPUT_PULL_UP,
                       pull_dir != BINARY_INPUT_PULL_UP);
    }
}

int binary_input_read(binary_input * b) {
    if (b->muxed) {
        uint8_t pin_mask = 0;
        for (uint8_t n = 0; n < b->num_pins; n++) {
            uint8_t p = gpio_get_w_pull_and_invert(b->pins[n],b->inverted);
            pin_mask = pin_mask | (p << n);
        }
        return pin_mask;
    } else {
        for (uint8_t n = 0; n < b->num_pins; n++) {
            if (gpio_get_w_pull_and_invert(b->pins[n],b->inverted)){
                return n+1;
            }
        }
        return 0;
    }
}