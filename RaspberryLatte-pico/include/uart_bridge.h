#ifndef _UART_BRIDGE_H
#define _UART_BRIDGE_H

#include "pico/stdlib.h"

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

typedef uint8_t message_id;
typedef uint8_t message_len;

/** 
 * \brief A function that handles UART messages with a certain ID.
 * 
 * \param local_data A pointer to a struct containing specific details needed by the callback
 * \param uart_data An int pointer to the data recieved over UART. Each integer containing one byte of data.
 * \param uart_data_len The number of bytes in the message's body 
 */
typedef void (*message_callback)(message_id id, void * local_data, int * uart_data, int uart_data_len);

typedef uint8_t MessageID;
typedef uint8_t MessageLen;

typedef struct{
    message_id id;
    message_callback callback;
    void * local_data;
} msg_handler;

/**
 * \brief Setup UART pins
 */
void uart_bridge_setup();

/**
 * \brief Define the indicated function as the message's handler
 * 
 * \param local_data A pointer to a struct containing specific details needed by the callback.
 * \param id ID of message to be handled. Value in [0,15].
 * \param callback A pointer to the callback function.
 * 
 * \returns 1 if messageID was unclaimed. 0 else. 
 */
int uart_bridge_register_handler(message_id id, void * local_data, message_callback callback);

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
 * \param status A status defined in errors.h
 * \param data Pointer to an int array of length \p len containing the data to be sent
 * \param len Integer giving the length of the \p data array.
 */
void sendMessageWithStatus(MessageID id, int status, int * data, int len);

#endif