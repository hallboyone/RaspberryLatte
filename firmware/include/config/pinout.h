/**
 * \file pinout.h
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Macros defining the pinout of the RaspberryLatte main board
 * \version 0.1
 * \date 2022-12-09
 */

#ifndef PINOUT_H
#define PINOUT_H

#define I2C_SDA_PIN           2 /**<\brief I2C data pin. */
#define I2C_SCL_PIN           3 /**<\brief I2C clock pin. */
#define SOLENOID_PIN          5 /**<\brief 3-way solenoid control pin. */
#define LMT01_DATA_PIN        6 /**<\brief LMT01 data pin. */
#define DIAL_A_PIN            7 /**<\brief Front dial pin A. */
#define DIAL_B_PIN            8 /**<\brief Front dial pin B. */
#define PUMP_SWITCH_PIN       9 /**<\brief Pump switch pin. */
#define PUMP_OUT_PIN         10 /**<\brief Pump control pin. */
#define HEATER_PWM_PIN       11 /**<\brief Boiler control pin. */
#define LED2_PIN             12 /**<\brief LED 2 control pin. */
#define LED1_PIN             13 /**<\brief LED 1 control pin. */
#define AC_0CROSS_PIN        14 /**<\brief Zerocross sensor pin. */
#define LED0_PIN             15 /**<\brief LED 0 control pin. */
#define FLOW_RATE_PIN        18 /**<\brief Flow rate data pin. */
#define PRESSURE_SENSOR_PIN  28 /**<\brief Analog pressure sensor pin. */
#endif