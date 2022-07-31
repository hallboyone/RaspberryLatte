#include <stdlib.h>
#include "binary_input.h"

#include <string.h>

#include "status_ids.h"
#include "uart_bridge.h"

#define MAX_NUM_BINARY_INPUTS 32

/**
 * \brief Data related to a binary input. Handles multithrow switches and allows for muxed hardware.
 */
typedef struct {
    uint8_t num_pins;
    uint8_t* pins;
    bool muxed;
    bool inverted;
} binary_input;

static binary_input _binary_inputs[MAX_NUM_BINARY_INPUTS];
static uint8_t _num_binary_inputs = 0;


/**
 * \brief Reads the indicated GPIO pin and inverts the results if the pin is pulled up. Inverts
 * again if invert is true.
 * 
 * \param pin_idx GPIO number to read
 * \param invert Inverts the result if true.
 * 
 * \returns !invert if pin reads opposite its pull. Otherwise returns invert.
 */
static inline uint8_t gpio_get_w_pull_and_invert(uint pin_idx, bool invert) {
    bool var = (gpio_is_pulled_down(pin_idx) ? gpio_get(pin_idx) : !gpio_get(pin_idx));
    return invert ? !var : var;
}

/**
 * \brief Read the physical inputs and return their values over UART. If no indexes are specified,
 * the states of all inputs are returned. Else, only the indicies in the message body are
 * returned. If an index is out of range, an IDX_OUT_OF_RANGE error is returned instead.
 *
 * \param data Pointer to switch indicies to read. If empty, return all.
 * \param len Number of indicies in data array.
 */
static void binary_input_read_handler(int* data, int len) {
    int status = SUCCESS;
    if (len == 0) {  // Read all switches in order they were added
        int response[_num_binary_inputs];
        for (uint8_t s_i = 0; s_i < _num_binary_inputs; s_i++) {
            response[s_i] = binary_input_read(s_i);
        }
        sendMessageWithStatus(MSG_ID_GET_SWITCH, status, response, _num_binary_inputs);
    } else {  // Read only the requested switches
        int response[len];
        for (uint8_t s_i = 0; s_i < len; s_i++) {
            if (data[s_i] < _num_binary_inputs) {
                response[s_i] = binary_input_read(data[s_i]);
            } else {
                response[s_i] = IDX_OUT_OF_RANGE;
                status = MSG_FORMAT_ERROR;
            }
        }
        sendMessageWithStatus(MSG_ID_GET_SWITCH, status, response, len);
    }
}

/**
 * \brief Create a binary input with the indicated number of throws. Only one of the throws
 * can be active at a time. The state of the switch is packed into binary indicating
 * the index of the active pin.
 *
 * \param num_pins The number of pins for the binary input.
 * \param pins Pointer to an array of GPIO pin numbers of length \p num_throw.
 * \param pull_dir Set to either PULL_UP or PULL_DOWN 
 * \param invert Flag indicating if the pins should be inverted. 
 * \param muxed True if digital inputs are muxed (e.g. 4 throw muxed into 2 pins)
 * 
 * \returns A unique ID assigned to the binary input or -1 if no input was created
 */
int binary_input_setup(uint8_t num_pins, const uint8_t * pins, uint8_t pull_dir, bool invert, bool muxed){
    if (_num_binary_inputs >= MAX_NUM_BINARY_INPUTS) {
        return -1;
    }
    // Copy data into next _binary_inputs slot.
    _binary_inputs[_num_binary_inputs].num_pins = num_pins;
    _binary_inputs[_num_binary_inputs].pins = (uint8_t*)malloc(sizeof(uint8_t) * num_pins);
    memcpy(_binary_inputs[_num_binary_inputs].pins, pins, num_pins);
    _binary_inputs[_num_binary_inputs].muxed = muxed;
    _binary_inputs[_num_binary_inputs].inverted = invert;

    // Setup each pin.
    for (uint8_t p = 0; p < _binary_inputs[_num_binary_inputs].num_pins; p++) {
        gpio_init(_binary_inputs[_num_binary_inputs].pins[p]);
        gpio_set_dir(_binary_inputs[_num_binary_inputs].pins[p], false);
        gpio_set_pulls(_binary_inputs[_num_binary_inputs].pins[p], pull_dir == BINARY_INPUT_PULL_UP,
                       pull_dir != BINARY_INPUT_PULL_UP);
    }

    _num_binary_inputs += 1;

    registerHandler(MSG_ID_GET_SWITCH, &binary_input_read_handler);

    return _num_binary_inputs-1;
}

/**
 * \brief Reads the requested switch. If switch is muxed, returns the bit mask of the pins. Else
 * returns the index of first high pin. 
 * 
 * \param switch_idx Index of the requested switch. Must have been setup previously. 
 * \returns If switch is not muxed, then the index of the first active pin is returned (1 indexed, 
 * 0 if no pin is found). If switch is muxed, then the the state of the pins are encoded into a 
 * uint8_t mask (i.e, second of three pins active, 010 returned).
 */
uint8_t binary_input_read(uint8_t switch_idx) {
    if(switch_idx < _num_binary_inputs){
        return 0;
    }

    if (_binary_inputs[switch_idx].muxed) {
        uint8_t pin_mask = 0;
        for (uint8_t n = 0; n < _binary_inputs[switch_idx].num_pins; n++) {
            uint8_t p = gpio_get_w_pull_and_invert(_binary_inputs[switch_idx].pins[n],
                                                   _binary_inputs[switch_idx].inverted);
            pin_mask = pin_mask | (p << n);
        }
        return pin_mask;
    } else {
        for (uint8_t n = 0; n < _binary_inputs[switch_idx].num_pins; n++) {
            if (gpio_get_w_pull_and_invert(_binary_inputs[switch_idx].pins[n],
                                           _binary_inputs[switch_idx].inverted)){
                return n + 1;
            }
        }
        return 0;
    }
}