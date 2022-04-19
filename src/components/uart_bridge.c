#include "uart_bridge.h"

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
int setHandler(MessageHandler h, MessageID id){
  assert(id<=15);
  if(handlers[id] != NULL) return 0;
  
  handlers[id] = h;
  return 1;
}

/**
 * Reads messages over the UART until empty or timeout is reached
 * 
 * @param timeout_us Value in microseconds before returning to main loop regardless of uart status
 * 
 * @returns 1 if unknown message was read. 0 else. 
 */
int readMessages(uint64_t timeout_us){
  // All messages start with 
  
}

void blink_io(){
  int timeout = 1000;
  int msg[2] = {0,0};
  while(true){
    msg[0] = getchar_timeout_us(0);
    if(msg[0] != PICO_ERROR_TIMEOUT){
      msg[1] = getchar_timeout_us(0);
      if(msg[1] != PICO_ERROR_TIMEOUT){
        timeout = msg[1]<<8 | msg[0];
      }
    }
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    sleep_ms(timeout);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    sleep_ms(timeout);
  }
}