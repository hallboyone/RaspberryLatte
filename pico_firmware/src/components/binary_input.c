#include <stdlib.h>
#include <string.h>

#include "pico/time.h"

#include "binary_input.h"
#include "status_ids.h"

/** \brief The binary input owning the corrisponding GPIO */
static binary_input * _binary_inputs [32] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                             NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                             NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                             NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

/** \brief Sets flag that indicates bouncing has ended
 * 
 * When a debounced pin transitions between states, a flag is set that indicates the pin recently
 * changed and an alarm is set that will call this callback to clear the alarm. If that alarm is 
 * not canceled (by another transition, for example), the binary_input has setteled and future read
 * operations will read the pins state instead of using the recorded ones
 */
static int64_t _debounce_callback(alarm_id_t id, void * b_in){
    binary_input * b = (binary_input*) b_in;
    b->bouncing = false;
}

/** \brief ISR for debouncing. */
static void _set_debounce_alarm(uint gpio, uint32_t events){
    _binary_inputs[gpio]->bouncing = true;
    if(_binary_inputs[gpio]->debounce_alarm != -1){
        cancel_alarm(_binary_inputs[gpio]->debounce_alarm);
    }
    _binary_inputs[gpio]->debounce_alarm = add_alarm_in_us(_binary_inputs[gpio]->debounce_us, &_debounce_callback, _binary_inputs[gpio], true);
}

/**
 * \brief If input is not bouncing, iterates through each pin and saves its state 
 * while taking into account pull direction and invertion.
 * 
 * \param b Pointer to binary_input object that will be updated.
 */
static inline void _binary_input_update_pin_states(binary_input * b){
    if (!b->bouncing){
        for(uint8_t p_idx = 0; p_idx < b->num_pins; p_idx++){
            uint8_t p = b->pins[p_idx];
            bool var = (gpio_is_pulled_down(p) ? gpio_get(p) : !gpio_get(p));
            b->pin_states[p_idx] = b->inverted ? !var : var;
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
    b->bouncing    = false;
    b->debounce_alarm = -1;

    // Init the allocated arrays
    memcpy(b->pins, pins, num_pins);
    for(uint i = 0; i < num_pins; i++) b->pin_states[i] = false;

    // Setup each pin.
    for (uint8_t p = 0; p < b->num_pins; p++) {
        gpio_init(b->pins[p]);
        gpio_set_dir(b->pins[p], false);
        gpio_set_pulls(b->pins[p], pull_dir == BINARY_INPUT_PULL_UP,
                       pull_dir != BINARY_INPUT_PULL_UP);
        _binary_inputs[b->pins[p]] = b;
        if(debounce_us > 0){
            // Attach irq to each pin if debouncing
            gpio_set_irq_enabled_with_callback(b->pins[p], GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &_set_debounce_alarm);
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