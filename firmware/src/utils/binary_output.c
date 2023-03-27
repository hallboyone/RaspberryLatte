/**
 * \file binary_output.c
 * \ingroup binary_output
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Binary Output source
 * \version 0.1
 * \date 2022-12-09
 * 
 */
#include "utils/binary_output.h"

#include <stdlib.h>
#include <string.h>

/**
 * \brief Struct containing information for a single output block.
 */
typedef struct binary_output_s {
    uint8_t num_pins; /**< The number of pins in output block. */
    uint8_t * pins;   /**< Pointer to array of pin numbers for output block. */
} binary_output_;

binary_output binary_output_setup(const uint8_t * pins, const uint8_t num_pins){
    binary_output b = malloc(sizeof(binary_output_));

    b->num_pins = num_pins;
    b->pins = (uint8_t*)malloc(sizeof(uint8_t) * num_pins);
    memcpy(b->pins, pins, num_pins);

    // Setup each pin.
    for (uint8_t p = 0; p < num_pins; p++) {
        gpio_init(b->pins[p]);
        gpio_set_dir(b->pins[p], true);
    }

    return b;
}

int binary_output_put(binary_output b, uint8_t idx, bool val){
    if(idx < b->num_pins){
        gpio_put(b->pins[idx], val);
        return 1;
    }
    return 0;
}

int binary_output_mask(binary_output b, uint mask){
    for(int i = 0; i < b->num_pins; i++){
        gpio_put(b->pins[b->num_pins-i-1], mask & 1);
        mask >>= 1;
    }
    return 1;
}


void binary_output_deinit(binary_output b){
    free(b);
}