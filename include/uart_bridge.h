#include "pico/stdlib.h"
#include "machine_structs.h"

typedef void (*MessageHandler)(int *, int);
typedef uint8_t MessageID;
typedef uint8_t MessageLen;
/**
 * Define the indicated function as the message's handler
 * 
 * @param h Pointer to function set to handle id
 * @param id ID of message to be handled. Value in [0,15].
 * 
 * @returns 1 if messageID was unclaimed. 0 else. 
 */
int setHandler(MessageID id, MessageHandler h);

/**
 * Reads messages over the UART until empty or timeout is reached
 * 
 * @param timeout_us Value in microseconds before returning to main loop regardless of uart status
 * 
 * @returns 1 if unknown message was read. 0 else. 
 */
int readMessages(uint64_t timeout_us);