#include <malloc.h>
#include <string.h>

#include "binary_inputs.h"
#include "uart_bridge.h"

static uint8_t * _binary_inputs[8] = {NULL, NULL, NULL, NULL,
                                      NULL, NULL, NULL, NULL};
static uint8_t _num_binary_inputs = 0;

/**
 * Create a binary input with the indicated number of throws. Only one of the throws
 * can be active at a time. The state of the switch is packed into binary indicating
 * the index of the active pin.
 *
 * @param num_throw The number of throws in the binary input.
 * @param pins Pointer to an array of GPIO pin numbers of length \p num_throw.
 * @param pull_down True if pins should be pulled down. False else. 
 */
void binary_inputs_setup(uint8_t num_throw, const uint8_t * pins, bool pull_down){
  assert(_num_binary_inputs < 8);
  
  // Copy data to _binary_inputs. Data is stored as [num_throw, pin1, pin2, ... , pin_num_throw]
  _binary_inputs[_num_binary_inputs] = (uint8_t*)malloc((1+num_throw)*sizeof(uint8_t));
  _binary_inputs[_num_binary_inputs][0] = num_throw;
  memcpy(_binary_inputs[_num_binary_inputs]+1, pins, num_throw);

  // Setup each pin. INdex shifted to skip leading num_throw value
  for (uint8_t p = 1; p <= _binary_inputs[_num_binary_inputs][0]; p++){
    gpio_init(_binary_inputs[_num_binary_inputs][p]);
    gpio_set_dir(_binary_inputs[_num_binary_inputs][p], false);
    gpio_set_pulls(_binary_inputs[_num_binary_inputs][p], !pull_down, pull_down);
  }

  _num_binary_inputs += 1;

  assignHandler(MSG_ID_READ_SWITCH, &physical_inputs_read_handler);
}

/**
 * Reads the requested switch.
 * 
 * @param switch_idx Index of the requested switch. Must have been setup previously. 
 * @returns 0 if non of the switches throws are triggered. Else returns the first triggered throw (Note: 1 indexed).
 */
uint8_t readSwitch(uint8_t switch_idx){
  // Make sure switch has been setup
  assert(switch_idx<_num_binary_inputs);

  uint8_t num_throws = _binary_inputs[switch_idx][0];
  for(uint8_t n = 1; n <= num_throws; n++){
    if(gpio_get(_binary_inputs[switch_idx][n])) return n;
  }
  return 0;
}

/**
 * Read the physical inputs and return their values over UART
 * 
 * @param data Pointer to switch indicies to read. If empty, return all.
 * @param len Number of indicies in data array. 
 */
static void physical_inputs_read_handler(int * data, int len){
  if(len == 0){
    // Read all switches in order they where added
    int response[_num_binary_inputs];
    for(uint8_t s_i = 0; s_i < _num_binary_inputs; s_i++){
      response[s_i] = readSwitch(s_i);
    }
    sendMessage(MSG_ID_READ_SWITCH, response, _num_binary_inputs);
  } else {
    // Read only the requested switches
    int response[len];
    for(uint8_t s_i = 0; s_i < len; s_i++){
      response[s_i] = readSwitch(data[s_i]);
    }
    sendMessage(MSG_ID_READ_SWITCH, response, len);
  }
}