#include <stdlib.h>
#include <string.h>

#include "binary_output.h"
#include "status_ids.h"

void binary_output_setup(binary_output * b, const uint8_t * pins, const uint8_t num_pins){
    b->num_pins = num_pins;
    b->pins = (uint8_t*)malloc(sizeof(uint8_t) * num_pins);
    memcpy(b->pins, pins, num_pins);

    // Setup each pin.
    for (uint8_t p = 0; p < num_pins; p++) {
        gpio_init(b->pins[p]);
        gpio_set_dir(b->pins[p], true);
    }
}

int binary_output_put(binary_output * b, uint8_t idx, bool val){
    if(idx < b->num_pins){
        gpio_put(b->pins[idx], val);
        return 1;
    }
    return 0;
}

int binary_output_mask(binary_output * b, uint mask){
    for(int i = 0; i < b->num_pins; i++){
        gpio_put(b->pins[i], mask & 1);
        mask >>= 1;
    }
    return 1;
}
