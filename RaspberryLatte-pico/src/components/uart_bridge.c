#include <stdio.h>
#include <stdlib.h>
#include "uart_bridge.h"
#include "pico/time.h"

#define NUM_HANDLERS 256
static msg_handler * _handlers = NULL;

/**
 * \brief Setup UART pins
 */
void uart_bridge_setup(){
    stdio_uart_init_full(PICO_DEFAULT_UART_INSTANCE, 115200, PICO_DEFAULT_UART_TX_PIN, PICO_DEFAULT_UART_RX_PIN);
    while(getchar_timeout_us(10) != PICO_ERROR_TIMEOUT) tight_loop_contents();
    if(_handlers != NULL){
        _handlers = malloc(sizeof(msg_handler)*NUM_HANDLERS);
        for(uint i = 0; i < NUM_HANDLERS; i++){
            _handlers[i].callback = NULL;
            _handlers[i].id = 0;
            _handlers[i].local_data = NULL;
        }
    }
}

/**
 * \brief Define the indicated function as the message's handler
 * 
 * \param local_data A pointer to a struct containing specific details needed by the callback.
 * \param id ID of message to be handled. Value in [0,15].
 * \param callback A pointer to the callback function.
 * 
 * \returns 1 if messageID was unclaimed. 0 else. 
 */
int uart_bridge_register_handler(message_id id, void * local_data, message_callback callback){
    if (_handlers == NULL){
        uart_bridge_setup();
    }
    _handlers[id].callback = callback;
    _handlers[id].id = id;
    _handlers[id].local_data = local_data;
}

/**
 * Read message over the UART if avalible
 * 
 * @returns -1 if unknown message was read, 0 if no message was found, and 1 if messge was handled. 
 */
int readMessage(){
  // Read message header. Return if none found
  int msg_header = getchar_timeout_us(0);
  if(msg_header == PICO_ERROR_TIMEOUT){
    // If no message was found
    return MSG_READ_FAIL_NO_MSG;
  } 

  // Parse header and check if message is valid
  MessageID id   = (msg_header & 0xF0)>>4;
  MessageLen len = (msg_header & 0x0F);
  if (_handlers[id].callback==NULL){
    // Unkown handler. Empty buffer and return.
    while(getchar_timeout_us(10) != PICO_ERROR_TIMEOUT) tight_loop_contents();
    return MSG_READ_FAIL_UNCONF_MSG;
  }
  
  // Get message data
  int msg_body[len];
  for(uint8_t n = 0; n < len; n++){
    if((msg_body[n]=getchar_timeout_us(5000)) == PICO_ERROR_TIMEOUT){
      // If not enough data, return -2
      return MSG_READ_FAIL_INVALID_MSG;
    }
  }

  // Call handler
  //handlers[id](msg_body, len);
  _handlers[id].callback(_handlers[id].local_data, msg_body, len);
  return MSG_READ_SUCCESS;
}

/**
 * Send message over the UART
 * 
 * \param id The message id that triggered the send command
 * \param data Pointer to an int array of length \p len containing the data to be sent
 * \param len Integer giving the length of the \p data array.
 */
void sendMessage(MessageID id, int * data, int len){
  putchar_raw((id<<4) | len);
  for(int n = 0; n<len; n++){
    putchar_raw(data[n]);
  }
}

/**
 * Send message over the UART
 * 
 * \param id The message id that triggered the send command
 * \param status A status defined in errors.h
 * \param data Pointer to an int array of length \p len containing the data to be sent
 * \param len Integer giving the length of the \p data array.
 */
void sendMessageWithStatus(MessageID id, int status, int * data, int len){
  putchar_raw((id<<4) | len);
  putchar_raw(status);
  for(int n = 0; n<len; n++){
    putchar_raw(data[n]);
  }
}