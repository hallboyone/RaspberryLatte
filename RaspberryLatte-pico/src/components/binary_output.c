#include <malloc.h>
#include <string.h>

#include "binary_output.h"
#include "uart_bridge.h"

static bool _binary_outputs[29];

/**
 * Handles messages to set GPIO outputs over uart. Messages have the form 
 *      [0-3:MSG_ID_SET_GPIO][4-7:len] - [8:val][9-15:pin_idx] - {[16:val][17-23:pin_idx]}...
 */
static void binary_output_read_handler(int * data, int len){
    assert(len>=1);
    for(uint byte_idx = 0; byte_idx < len; byte_idx++){
        uint gpio_num = data[0] & 0x7F;
        uint val = data[0]   & 0x80;
        binary_output_write(gpio_num, val)
        assert(gpio_num<29);
        if(_binary_outputs[gpio_num]){
            gpio_put(gpio_num, val);
        }
    }
}

/**
 * Create a binary output attached to the indicated pin. The first time this function is called, a binary output write 
 * handler is registered with the uart bridge. 
 */
void binary_output_setup(const uint gpio_num){
    assert(gpio_num < 29);
    if(!_binary_outputs[gpio_num]){
        _binary_outputs[gpio_num] = true;
        gpio_init(gpio_num);
        gpio_set_dir(gpio_num, true)
        gpio_put(gpio_num, 0);
        assignHandler(MSG_ID_SET_GPIO, &binary_output_read_handler);
    }
}

/**
 * Reads the requested switch. If switch is muxed, returns the bit mask of the pins. Else returns the index of first high pin
 * 
 * @param switch_idx Index of the requested switch. Note this is not a GPIO number!
 * @param val Value to write to output.
  */
void binary_output_write(uint gpio_num, bool val){
    assert(gpio_num<29);
    if(_binary_outputs[gpio_num]){
        gpio_put(gpio_num, val);
    }
}