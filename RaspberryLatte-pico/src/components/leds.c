#include "leds.h"
#include "uart_bridge.h"

static uint _led_pins[3];
/**
 * Handles messages to set GPIO outputs over uart. Messages have the form 
 *      [0-3:MSG_ID_SET_GPIO][4-7:len] - [8:val][9-15:pin_idx] - {[16:val][17-23:pin_idx]}...
 */
static void leds_set_handler(int * data, int len){
    assert(len>0 && len < 3);
    for(uint byte_idx = 0; byte_idx < len; byte_idx++){
        uint led_idx = data[byte_idx] & 0x7F;
        uint val = data[byte_idx]   & 0x80;
        leds_set(led_idx, val);
    }
}

/**
 * Create a binary output attached to the indicated pin. The first time this function is called, a binary output write 
 * handler is registered with the uart bridge. 
 */
void leds_setup(const uint led0_pin, const uint led1_pin, const uint led2_pin){
    _led_pins[0] = led0_pin;
    _led_pins[1] = led1_pin;
    _led_pins[2] = led2_pin;
    for(uint i = 0; i<3; i++){
        gpio_init(_led_pins[i]);
        gpio_set_dir(_led_pins[i], true);
        gpio_put(_led_pins[i], 0);
    }
    registerHandler(MSG_ID_SET_LEDS, &leds_set_handler);
}

/**
 * Reads the requested switch. If switch is muxed, returns the bit mask of the pins. Else returns the index of first high pin
 * 
 * @param switch_idx Index of the requested switch. Note this is not a GPIO number!
 * @param val Value to write to output.
  */
void leds_set(uint led_idx, bool val){
    assert(led_idx<3);
    gpio_put(_led_pins[led_idx], val);
}