#include <malloc.h>
#include <string.h>

#include "binary_input.h"
#include "uart_bridge.h"
#include "errors.h"

static uint8_t * _binary_inputs[8] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
static uint8_t _num_binary_inputs = 0;

#define NUM_PIN_IDX   0
#define MUXED_IDX     1
#define PIN_OFFSET    2

/**
 * @brief Reads the indicated GPIO pin and inverts the results if the pin is pulled up
 * 
 * @param pin_idx GPIO number to read
 * @return True if pin is high and pulled down or pin is low and pulled up. False otherwise.
 */
static inline uint8_t gpio_get_w_pull(uint pin_idx){
  return (gpio_is_pulled_down(pin_idx) ? gpio_get(pin_idx) : !gpio_get(pin_idx));
}

/**
 * Read the physical inputs and return their values over UART. If no indexes are specified,
 * the states of all inputs are returned. Else, only the indicies in the message body are
 * returned. If an index is out of range, an IDX_OUT_OF_RANGE error is returned instead.
 * 
 * @param data Pointer to switch indicies to read. If empty, return all.
 * @param len Number of indicies in data array. 
 */
static void binary_input_read_handler(int * data, int len){
  if(len == 0){ // Read all switches in order they were added
    int response[_num_binary_inputs];
    for(uint8_t s_i = 0; s_i < _num_binary_inputs; s_i++){
      response[s_i] = binary_input_read(s_i);
    }
    sendMessage(MSG_ID_GET_SWITCH, response, _num_binary_inputs);
  } else { // Read only the requested switches
    int response[len];
    for(uint8_t s_i = 0; s_i < len; s_i++){
      if (data[s_i] < _num_binary_inputs){
        response[s_i] = binary_input_read(data[s_i]);
      } else {
        response[s_i] = IDX_OUT_OF_RANGE;
      }
    }
    sendMessage(MSG_ID_GET_SWITCH, response, len);
  }
}

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
void binary_input_setup(uint8_t num_pins, const uint8_t * pins, uint8_t pull_dir, bool muxed){
  assert(_num_binary_inputs < 8);
  // Copy data to _binary_inputs. Data is stored as [num_pins, pull_down, muxed, pin1, pin2, ... , pin_num_throw]
  _binary_inputs[_num_binary_inputs] = (uint8_t*)malloc((3+num_pins)*sizeof(uint8_t));
  _binary_inputs[_num_binary_inputs][NUM_PIN_IDX] = num_pins;
  _binary_inputs[_num_binary_inputs][MUXED_IDX] = muxed;
  memcpy(_binary_inputs[_num_binary_inputs]+PIN_OFFSET, pins, num_pins);

  // Setup each pin.
  for (uint8_t p = 0; p < _binary_inputs[_num_binary_inputs][NUM_PIN_IDX]; p++){
    gpio_init(_binary_inputs[_num_binary_inputs][p+PIN_OFFSET]);
    gpio_set_dir(_binary_inputs[_num_binary_inputs][p+PIN_OFFSET], false);
    gpio_set_pulls(_binary_inputs[_num_binary_inputs][p+PIN_OFFSET], pull_dir==PULL_UP, pull_dir!=PULL_UP);
  }

  _num_binary_inputs += 1;

  registerHandler(MSG_ID_GET_SWITCH, &binary_input_read_handler);
}

/**
 * Reads the requested switch. If switch is muxed, returns the bit mask of the pins. Else returns the index of first high pin
 * 
 * @param switch_idx Index of the requested switch. Must have been setup previously. 
 * @returns If switch is not muxed, then the index of the first active pin is returned (1 indexed, 0 if no pin is found)
 * If switch is muxed, then the the state of the pins are encoded into a uint8_t mask (i.e, second of three pins active, 0b10 returned)
 */
uint8_t binary_input_read(uint8_t switch_idx){
  // Make sure switch has been setup
  assert(switch_idx<_num_binary_inputs);

  if(_binary_inputs[switch_idx][MUXED_IDX]){ // If muxed,
  uint8_t pin_mask = 0;
    for(uint8_t n = 0; n < _binary_inputs[switch_idx][NUM_PIN_IDX]; n++){
      pin_mask = pin_mask | (gpio_get_w_pull(_binary_inputs[switch_idx][n+PIN_OFFSET])<<n);
    }
    return pin_mask;
  } else{
    for(uint8_t n = 0; n < _binary_inputs[switch_idx][NUM_PIN_IDX]; n++){
      if(gpio_get_w_pull(_binary_inputs[switch_idx][n+PIN_OFFSET])) return n+1;
    }
    return 0;
  }
}