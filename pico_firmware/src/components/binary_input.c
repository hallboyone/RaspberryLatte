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

/**
 * \brief Create a binary input with the indicated number of throws. Only one of the throws
 * can be active at a time. The state of the switch is packed into binary indicating
 * the index of the active pin.
 *
 * \param num_pins The number of pins for the binary input.
 * \param pins Pointer to an array of GPIO pin numbers of length \p num_throw.
 * \param pull_dir Set to either PULL_UP or PULL_DOWN 
 * \param invert Flag indicating if the pins should be inverted. 
 * \param muxed True if digital inputs are muxed (e.g. 4 throw muxed into 2 pins)
 * 
 * \returns A unique ID assigned to the binary input or -1 if no input was created
 */
int binary_input_setup(binary_input * b, uint8_t num_pins, const uint8_t * pins, uint8_t pull_dir, bool invert, bool muxed){
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
    return 1;
}

/**
 * \brief Reads the requested switch. If switch is muxed, returns the bit mask of the pins. Else
 * returns the index of first high pin. 
 * 
 * \param switch_idx Index of the requested switch. Must have been setup previously. 
 * 
 * \returns If switch is not muxed, then the index of the first active pin (1-indexed) is returned 
 * or 0 if no active pin. If switch is muxed, then the the state of the pins are encoded 
 * into a uint8_t mask (i.e, second of three pins active, 010 returned).
 */
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