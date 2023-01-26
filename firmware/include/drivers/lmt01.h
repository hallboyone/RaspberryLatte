/**
 * \defgroup lmt01 LMT01 Driver
 * \ingroup drivers
 * \brief Driver to interface with a LMT01 sensor using a PIO program.
 * 
 * The LMT01 is a high precision temperature sensor that periodically sends a sequence current
 * pulses. The number of pulses is roughly proportional to the current temperature. This driver
 * counts the number of pulses using a PIO program and converts this to the corrsponding
 * temperature using linear interpolation between data points givin in the data sheet.
 * 
 * \{
 * \file
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief LMT01 Driver header
 * \version 0.1
 * \date 2022-08-16
 */


#ifndef LMT01_H
#define LMT01_H

#include "pico/stdlib.h"
#include "hardware/pio.h"

/**
 * \brief Structure for interfacing with a single LMT01 sensor.
 */
typedef struct{
    PIO _pio;         /**< The PIO instance running the program reading the LMT01 sensor. */
    uint _sm;         /**< The state machine instance running the program reading the LMT01 sensor. */
    uint8_t _dat_pin; /**< The GPIO pin attached to the LMT01 signal. */
    int _latest_temp; /**< The most recent temp read by the sensor. */
} lmt01;

/**
 * \brief Configures the signal pin attached to a LMT01 temp sensor and starts a PIO 
 * program that counts the sensors pulse train
 * 
 * \param l Pointer to lmt01 struct that will contain the required parameters for the LMT01 interface
 * \param pio_num Either 0 or 1 indicating if PIO #0 or #1 should be used
 * \param dat_pin Pin that the LMT01 is attached to
 */
void lmt01_setup(lmt01 * l, uint8_t pio_num, uint8_t dat_pin);

/**
 * \brief Returns the current temperature in 16*C. Divide by 16 to convert to C
 * 
 * \param l Pointer to lmt01 struct that contains the parameters for the LMT01 interface
 * 
 * \returns The current temperature in 16*C.
 */
int lmt01_read(lmt01 * l);

/**
 * \brief Returns the current temperature in C.
 * 
* \param l Pointer to lmt01 struct that contains the parameters for the LMT01 interface
 * 
 * \returns The current temperature in C.
 */
float lmt01_read_float(lmt01 * l);
#endif
/** \} */