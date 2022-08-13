#include <stdlib.h>
#include <string.h>

#include "binary_output.h"
#include "status_ids.h"

/**
 * \brief Setup a bank of binary outputs with 1 or more pins.
 *
 * \param b Pointer to binary_output object that will be setup.
 * \param pins Pointer to an array of GPIO pin numbers.
 * \param num_pins The number of pins for the binary output.
 * 
 * \returns 1 for success. 0 for failure.
 */
int binary_output_setup(binary_output * b, const uint8_t * pins, const uint8_t num_pins){
    b->num_pins = num_pins;
    b->pins = (uint8_t*)malloc(sizeof(uint8_t) * num_pins);
    memcpy(b->pins, pins, num_pins);

    // Setup each pin.
    for (uint8_t p = 0; p < num_pins; p++) {
        gpio_init(b->pins[p]);
        gpio_set_dir(b->pins[p], true);
    }
    return 1;
}

/**
 * \brief Write val to the specified GPIO pin.
 * 
 * \param id ID of block containing desired GPIO pin.
 * \param offset Offset into the block containing the desired GPIO pin.
 * \param val Binary value to write to GPIO.
 * 
 * \returns 1 if operation was successful. 0 else.
 */
int binary_output_put(binary_output * b, uint8_t idx, bool val){
    if(idx < b->num_pins){
        gpio_put(b->pins[idx], val);
        return 1;
    }
    return 0;
}

/**
 * \brief Write the mask to the specified output block. 
 * 
 * \param id ID of block to write to.
 * \param mask Binary values to write to block.
 */
int binary_output_mask(binary_output * b, uint mask){
    for(int i = 0; i < b->num_pins; i++){
        gpio_put(b->pins[i], mask & 1);
        mask >>= 1;
    }
    return 1;
}

void binary_output_uart_callback(message_id id, void * local_data, int * uart_data, int uart_data_len){
    if(uart_data_len % 2 != 0){
        sendMessageWithStatus(id, MSG_FORMAT_ERROR, NULL, 0);
        return;
    } else {
        for(int n = 0; n<uart_data_len; n += 2){
            if(!binary_output_put((binary_output *)local_data, uart_data[n], uart_data[n+1])){
                sendMessageWithStatus(id, IDX_OUT_OF_RANGE, NULL, 0);
                return;
            }
        }
        sendMessageWithStatus(id, SUCCESS, NULL, 0);
        return;
    }
}