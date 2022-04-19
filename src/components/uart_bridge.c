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
int setHandler(MessageID id, MessageHandler h){
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
  // Get absolute timeout time
  absolute_time_t end_time = make_timeout_time_us(timeout_us);
  int read_unknown = 0;
  
  // As long as there is still time
  for(int i = 0; i<5; i++){
    // Read message header. Return if none found
    int msg_header = getchar_timeout_us(0);
    if(msg_header == PICO_ERROR_TIMEOUT){
      return read_unknown;
    }

    // Parse header and read associated data
    MessageID id   = (msg_header & 0xF0)>>4;
    MessageLen len = (msg_header & 0x0F);
    int msg_body[len];
    for(uint8_t n = 0; n < len; n++){
      assert((msg_body[n]=getchar_timeout_us(10)) != PICO_ERROR_TIMEOUT);
    }

    // Call handler
    if(handlers[id]!=NULL){
      handlers[id](msg_body, len);
    } else {
      read_unknown = 1;
    }
  }
  return read_unknown;
}

// void blink_io(){
//   int timeout = 1000;
//   int msg[2] = {0,0};
//   while(true){
//     msg[0] = getchar_timeout_us(0);
//     if(msg[0] != PICO_ERROR_TIMEOUT){
//       msg[1] = getchar_timeout_us(0);
//       if(msg[1] != PICO_ERROR_TIMEOUT){
//         timeout = msg[1]<<8 | msg[0];
//       }
//     }
//     gpio_put(PICO_DEFAULT_LED_PIN, 1);
//     sleep_ms(timeout);
//     gpio_put(PICO_DEFAULT_LED_PIN, 0);
//     sleep_ms(timeout);
//   }
// }