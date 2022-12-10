/**
 * \defgroup flow_meter Flow Meter Driver
 * \ingroup drivers
 * 
 * \brief Driver using pulse-count flow meters
 * 
 * Tracks the number of ticks throw a flow meter and uses a \ref discrete_derivative to convert
 * to a rate of change (i.e. flow-rate). Methods are provided for reading both the volume of 
 * flow and the flowrate.
 * 
 * \{
 * 
 * \file flow_meter.h
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Flow Meter driver header
 * \version 0.1
 * \date 2022-12-10
*/
#ifndef FLOW_METER_H
#define FLOW_METER_H

#include "pico/stdlib.h"

#include "pid.h"

/** \brief Structure managing a single flowmeter. */
typedef struct {
    uint8_t pin;                   /**< \brief The GPIO attached to the flow meter's signal. */
    float conversion_factor;       /**< \brief Factor converting pulse counts to ml. */
    uint pulse_count;              /**< \brief Number of pulses since last zero. */
    discrete_derivative flow_rate; /**< \brief Derivative structure for tracking the flow rate. */
} flow_meter;

/**
 * \brief Configures a single flow_meter structure.
 * 
 * Runs three main tasks
 * -# Sets up the pin as a digital input that is pulled down. 
 * -# Attaches callback using \ref gpio_multi_callback.
 * -# Initializes the internal discrete_derivative.
 * 
 * \param fm The flow_meter that will be configured
 * \param pin_num The GPIO attached to the flow meter's signal wire
 * \param conversion_factor The factor to convert from pulse counts to ml
 * \return PICO_ERROR_NONE on success. Else, error code.  
 */
int flow_meter_setup(flow_meter * fm, uint8_t pin_num, float conversion_factor);

/**
 * \brief Return the volume since the last zero point
 * 
 * \param fm Flow meter to read
 * \return Volume, in ml, since last zero point.
 */
float flow_meter_volume(flow_meter * fm){
    return fm->pulse_count * fm->conversion_factor;
}

/**
 * \brief Returns the current flowrate of the sensor
 * 
 * \param fm Flow meter to read from
 * \return Flow rate in ml/s 
 */
float flow_meter_rate(flow_meter * fm){
    datapoint dp = {.t = sec_since_boot(), .v = flow_meter_volume(fm)};
    return discrete_derivative_add_point(&(fm->flow_rate), dp);
}

/**
 * \brief Returns the volume and flow rate to 0
 * 
 * \param fm Flow meter to zero. 
 */
void flow_meter_zero(flow_meter * fm){
    fm->pulse_count = 0;
    discrete_derivative_reset(&(fm->flow_rate));
}

#endif
/** \} */