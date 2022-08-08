#ifndef _MESSAGE_IDS_H
#define _MESSAGE_IDS_H

#define MSG_ID_END_PROGRAM   0 /**< Breaks the infinite while loop and exits the firmware */

#define MSG_ID_SET_LEDS      1
#define MSG_ID_SET_BIN_OUT   1
#define MSG_ID_SET_PUMP      2
#define MSG_ID_SET_SOLENOID  3
#define MSG_ID_SET_HEATER    4

#define MSG_ID_GET_SWITCH    8
#define MSG_ID_GET_PRESSURE  9
#define MSG_ID_GET_WEIGHT   10 
#define MSG_ID_GET_TEMP     11
#define MSG_ID_GET_AC_ON    12 /**< Returns true if the AC is on */

#endif