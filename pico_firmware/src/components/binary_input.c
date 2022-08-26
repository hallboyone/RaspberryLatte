#include <stdlib.h>
#include <string.h>

#include "pico/time.h"

#include "binary_input.h"
#include "status_ids.h"

/** \brief The absolute time points when binary input last changed */
static volatile absolute_time_t _trigger_times [32];

/** \brief ISR for debouncing. Records the transition time of each pin */
static void _save_trigger_time(uint gpio, uint32_t events){
    _trigger_times[gpio] = get_absolute_time();
}

/**
 * \brief Iterates through each pin and saves its state taking into account pull direction, invertion, and debouncing logic.
 * 
 * \param b Pointer to binary_input object that will be updated.
 */
static inline void _binary_input_update_pin_states(binary_input * b){
    absolute_time_t cur_t = get_absolute_time();
    for(uint8_t p = 0; p<b->num_pins; p++){
        if (b->debounce_us == 0 || absolute_time_diff_us(_trigger_times[b->pins[p]], cur_t) >= b->debounce_us){
            // Debounce not active or satisfied. Save pin state.
            bool var = (gpio_is_pulled_down(p) ? gpio_get(p) : !gpio_get(p));
            b->pin_states[p] = b->inverted ? !var : var;
        }
    }
}

void binary_input_setup(binary_input * b, uint8_t num_pins, const uint8_t * pins, uint8_t pull_dir, uint debounce_us, bool invert, bool muxed){
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
            gpio_set_irq_callback(_save_trigger_time);
            gpio_set_irq_enabled(b->pins[p], GPIO_IRQ_EDGE_FALL || GPIO_IRQ_EDGE_RISE, true);
            _trigger_times[b->pins[p]] = get_absolute_time();
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