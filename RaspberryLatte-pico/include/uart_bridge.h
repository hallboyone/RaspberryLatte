#include "pico/stdlib.h"

#define MSG_ID_END_PROGRAM   0 /**< Breaks the infinite while loop and exits the firmware */

#define MSG_ID_SET_LEDS      1
#define MSG_ID_SET_PUMP      2
#define MSG_ID_SET_SOLENOID  3
#define MSG_ID_SET_HEATER    4

#define MSG_ID_GET_SWITCH    8
#define MSG_ID_GET_PRESSURE  9
#define MSG_ID_GET_WEIGHT   10 
#define MSG_ID_GET_TEMP     11
#define MSG_ID_GET_AC_ON    12 /**< Returns true if the AC is on */


#define MSG_READ_SUCCESS           1
#define MSG_READ_FAIL_NO_MSG       0
#define MSG_READ_FAIL_UNCONF_MSG  -1
#define MSG_READ_FAIL_INVALID_MSG -2

/** 
 * \brief A function that handles UART messages with a certain ID.
 * 
 * \param data An int pointer with each element containing one byte of data.
 * \param len The number of bytes in the message's body 
 */
typedef void (*MessageHandler)(int * data, int len);

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
int registerHandler(MessageID id, MessageHandler h);

/**
 * Read message over the UART if avalible
 * 
 * @returns -2 if invalid message was read, -1 if unknown message was read, 
 * 0 if no message was found, and 1 if messge was handled. 
 */
int readMessage();

/**
 * Send message over the UART
 * 
 * \param id The message id that triggered the send command
 * \param data Pointer to an int array of length \p len containing the data to be sent
 * \param len Integer giving the length of the \p data array.
 */
void sendMessage(MessageID id, int * data, int len);

/**
 * Send message over the UART
 * 
 * \param id The message id that triggered the send command
 * \param status A status defined in errors.h
 * \param data Pointer to an int array of length \p len containing the data to be sent
 * \param len Integer giving the length of the \p data array.
 */
void sendMessageWithStatus(MessageID id, int status, int * data, int len);