#include <malloc.h>
#include <string.h>

#include "binary_output.h"
#include "uart_bridge.h"

static uint _binary_outputs[32];
static uint _num_binary_inputs = 0;

/**
 * Handles messages to set GPIO outputs over uart. Messages have the form 
 *      [0-3:MSG_ID_SET_GPIO][4-7:len] - [8:val][9-15:pin_idx] - {[16:val][17-23:pin_idx]}...
 */
static void binary_output_read_handler(int * data, int len){
    assert(len>=1);
    for(uint byte_idx = 0; byte_idx < len; byte_idx++){
        uint p_idx = data[0] & 0x7F;
        uint val = data[0]   & 0x80;
        assert(p_idx < _num_binary_inputs);
        gpio_put(_binary_outputs[p_idx], val);
    }
}

/**
 * Create a binary output attached to the indicated pin. The first time this function is called, a binary output write 
 * handler is registered with the uart bridge. 
 */
void binary_output_setup(const uint pin){
    assert(_num_binary_inputs < 32);
    gpio_init(pin);
    gpio_set_dir(pin, true)
    _binary_outputs[_num_binary_inputs] = pin;
    _num_binary_inputs += 1;
    
    assignHandler(MSG_ID_SET_GPIO, &binary_output_read_handler);
}

/**
 * Reads the requested switch. If switch is muxed, returns the bit mask of the pins. Else returns the index of first high pin
 * 
 * @param switch_idx Index of the requested switch. Note this is not a GPIO number!
 * @param val Value to write to output.
  */
void binary_output_write(uint switch_idx, bool val){
    assert(switch_idx<_num_binary_inputs);
    gpio_put(_binary_outputs[switch_idx], val);
}