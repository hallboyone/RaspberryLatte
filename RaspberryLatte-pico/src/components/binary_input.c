#include <malloc.h>
#include <string.h>

#include "binary_input.h"
#include "uart_bridge.h"

static uint8_t * _binary_inputs[8] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
static uint8_t _num_binary_inputs = 0;

/**
 * Create a binary input with the indicated number of throws. Only one of the throws
 * can be active at a time. The state of the switch is packed into binary indicating
 * the index of the active pin.
 *
 * @param num_pins The number of throws in the binary input.
 * @param pins Pointer to an array of GPIO pin numbers of length \p num_throw.
 * @param pull_down True if pins should be pulled down. False else. 
 * @param muxed True if digital inputs are muxed (e.g. 4 throw muxed into 2 pins)
 */
void binary_input_setup(uint8_t num_pins, const uint8_t * pins, bool pull_down, bool muxed){
  assert(_num_binary_inputs < 8);
  
  // Copy data to _binary_inputs. Data is stored as [num_pins, muxed, pin1, pin2, ... , pin_num_throw]
  _binary_inputs[_num_binary_inputs] = (uint8_t*)malloc((2+num_pins)*sizeof(uint8_t));
  _binary_inputs[_num_binary_inputs][0] = num_pins;
  _binary_inputs[_num_binary_inputs][1] = muxed;
  memcpy(_binary_inputs[_num_binary_inputs]+2, pins, num_pins);

  // Setup each pin. Index shifted to skip leading meta data
  for (uint8_t p = 0; p < _binary_inputs[_num_binary_inputs][0]; p++){
    gpio_init(_binary_inputs[_num_binary_inputs][p+2]);
    gpio_set_dir(_binary_inputs[_num_binary_inputs][p+2], false);
    gpio_set_pulls(_binary_inputs[_num_binary_inputs][p+2], !pull_down, pull_down);
  }

  _num_binary_inputs += 1;

  assignHandler(MSG_ID_GET_SWITCH, &binary_input_read_handler);
}

/**
 * Reads the requested switch. If switch is muxed, returns the bit mask of the pins. Else returns the index of first high pin
 * 
 * @param switch_idx Index of the requested switch. Must have been setup previously. 
 * @returns 0 if non of the switches throws are triggered. Else returns the first triggered throw (Note: 1 indexed).
 */
uint8_t readSwitch(uint8_t switch_idx){
  // Make sure switch has been setup
  assert(switch_idx<_num_binary_inputs);

  uint8_t num_pins = _binary_inputs[switch_idx][0];
  if(_binary_inputs[switch_idx][1]){ // If muxed,
    uint8_t pin_mask = 0;
    for(uint8_t n = 0; n < num_pins; n++){
      pin_mask = pin_mask || (gpio_get(_binary_inputs[switch_idx][n+2])<<n);
    }
    return pin_mask;
  } else{
    for(uint8_t n = 0; n < num_pins; n++){
      if(gpio_get(_binary_inputs[switch_idx][n+2])) return n;
    }
    return 0;
  }
}

/**
 * Read the physical inputs and return their values over UART
 * 
 * @param data Pointer to switch indicies to read. If empty, return all.
 * @param len Number of indicies in data array. 
 */
static void binary_input_read_handler(int * data, int len){
  if(len == 0){
    // Read all switches in order they were added
    int response[_num_binary_inputs];
    for(uint8_t s_i = 0; s_i < _num_binary_inputs; s_i++){
      response[s_i] = readSwitch(s_i);
    }
    sendMessage(MSG_ID_GET_SWITCH, response, _num_binary_inputs);
  } else {
    // Read only the requested switches
    int response[len];
    for(uint8_t s_i = 0; s_i < len; s_i++){
      response[s_i] = readSwitch(data[s_i]);
    }
    sendMessage(MSG_ID_GET_SWITCH, response, len);
  }
}