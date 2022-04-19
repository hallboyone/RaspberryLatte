#include <stdio.h>
#include "uart_bridge.h"
#include "pico/time.h"

/** Static array of the functions set to handle each of the 16 message types*/
static MessageHandler handlers[16] = {NULL,NULL,NULL,NULL,
                                      NULL,NULL,NULL,NULL,
                                      NULL,NULL,NULL,NULL,
                                      NULL,NULL,NULL,NULL};

/**
 * Define the indicated function as the message's handler
 * 
 * @param h Pointer to function set to handle id
 * @param id ID of message to be handled. Value in [0,15].
 * 
 * @returns 1 if messageID was unclaimed. 0 else. 
 */
int assignHandler(MessageID id, MessageHandler h){
  assert(id<=15);
  if(handlers[id] != NULL) return 0;
  
  handlers[id] = h;
  return 1;
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
    return 0;
  }

  // Parse header and read associated data
  MessageID id   = (msg_header & 0xF0)>>4;
  MessageLen len = (msg_header & 0x0F);
  int msg_body[len];
  for(uint8_t n = 0; n < len; n++){
    assert((msg_body[n]=getchar_timeout_us(10)) != PICO_ERROR_TIMEOUT);
  }

  if (handlers[id]==NULL){
    // Unkown handler
    return -1;
  } else {
    // Call Handler
    handlers[id](msg_body, len);
  }
  return 1;
}

void sendMessage(MessageID id, int * data, int len){
  putchar_raw((id<<4) | len);
  for(int n = 0; n<len; n++){
    putchar_raw(data[n]);
  }
}