#include <stdlib.h>
#include <string.h>

#include "binary_output.h"
#include "uart_bridge.h"
#include "status_ids.h"

#define MAX_NUM_BINARY_OUTPUTS 32

typedef struct {
    uint8_t num_pins;
    uint8_t * pins;
} binary_output;

static binary_output _binary_outputs[MAX_NUM_BINARY_OUTPUTS];
static uint8_t _num_binary_outputs = 0;

/**
 * \brief Write to binary outputs over UART. Each write takes three bytes in the form 
 * [BLOCK_ID]-[PIN_ID]-[VALUE]. A SUCCESS messege is returned if the messege is valid.
 * Else, a IDX_OUT_OF_RANGE messege is returned.
 *
 * \param data Pointer to 3n bytes indicating the block id, pin offset, and value of each consecutive write.
 * \param len Number of indicies in data array.
 */
static void binary_output_read_handler(int* data, int len) {
    if(len%3 != 0){
        sendMessageWithStatus(MSG_ID_SET_BIN_OUT, MSG_FORMAT_ERROR, NULL, 0);
        return;
    }
    for(int n = 0; n<len; n += 3){
        if(!binary_output_put(data[n], data[n+1], data[n+2])){
            sendMessageWithStatus(MSG_ID_SET_BIN_OUT, IDX_OUT_OF_RANGE, NULL, 0);
            return;
        }
    }
    sendMessageWithStatus(MSG_ID_SET_BIN_OUT, SUCCESS, NULL, 0);
}

/**
 * \brief Setup a bank of binary outputs with 1 or more pins.
 *
 * \param pins Pointer to an array of GPIO pin numbers.
 * \param num_pins The number of pins for the binary output.
 * 
 * \returns A unique, ID assigned to the binary output. -1 if output not created.
 */
int binary_output_setup(const uint8_t * pins, const uint8_t num_pins){
    if (_num_binary_outputs >= MAX_NUM_BINARY_OUTPUTS) {
        return -1;
    }
    _binary_outputs[_num_binary_outputs].num_pins = num_pins;
    _binary_outputs[_num_binary_outputs].pins = (uint8_t*)malloc(sizeof(uint8_t) * num_pins);
    memcpy(_binary_outputs[_num_binary_outputs].pins, pins, num_pins);

    // Setup each pin.
    for (uint8_t p = 0; p < num_pins; p++) {
        gpio_init(_binary_outputs[_num_binary_outputs].pins[p]);
        gpio_set_dir(_binary_outputs[_num_binary_outputs].pins[p], true);
    }

    _num_binary_outputs += 1;

    registerHandler(MSG_ID_SET_BIN_OUT, &binary_output_read_handler);

    return _num_binary_outputs-1;
}

/**
 * \brief Write val to the specified GPIO pin.
 * 
 * \param id ID of block containing desired GPIO pin.
 * \param offset Offset into the block containing the desired GPIO pin.
 * \param val Binary value to write to GPIO.
 * 
 * \returns True if operation was successful. False else.
 */
bool binary_output_put(uint8_t id, uint8_t offset, bool val){
    if(id < _num_binary_outputs){
        if(offset < _binary_outputs[id].num_pins){
            gpio_put(_binary_outputs[id].pins[offset], val);
            return true;
        }
    }
    return false;
}

/**
 * \brief Write the mask to the specified output block. 
 * 
 * \param id ID of block to write to.
 * \param mask Binary values to write to block.
 */
void binary_output_mask(uint8_t id, uint mask){
    if(id < _num_binary_outputs){
        for(int i = 0; i < _binary_outputs[id].num_pins; i++){
            gpio_put(_binary_outputs[id].pins[i], mask & 1);
            mask >>= 1;
        }
    }
}