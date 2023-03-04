/**
 * \defgroup flow_meter Flow Meter Driver
 * \ingroup drivers
 * 
 * \brief Driver for flow meters with volumetric pulsing output.
 * 
 * Tracks the number of ticks through a flow meter and uses a \ref discrete_derivative to convert
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

#include "utils/pid.h"

/** \brief Opaque object defining a single flow meter. */
typedef struct flow_meter_s* flow_meter;

/**
 * \brief Configures a single flow_meter structure.
 * 
 * Runs three main tasks
 * -# Sets up the pin as a digital input that is pulled down. 
 * -# Attaches callback using \ref gpio_multi_callback.
 * -# Initializes the internal discrete_derivative.
 * 
 * \param pin_num The GPIO attached to the flow meter's signal wire.
 * \param conversion_factor The factor to convert from pulse counts to desired volume.
 * \param filter_span_ms The duration over which the slope will be computed.
 * \param sample_dwell_time_ms The minimum duration between samples.
 * \return A new flow_meter object on success. NULL on error.  
 */
flow_meter flow_meter_setup(uint8_t pin_num, uint16_t conversion_factor, 
                            uint16_t filter_span_ms, uint16_t sample_dwell_time_ms);

/**
 * \brief Return the volume since the last zero point.
 * 
 * \param fm Flow meter to read volume from.
 * \return Volume since last zero point based on the conversion factor.
 */
uint flow_meter_volume(flow_meter fm);

/**
 * \brief Returns the current flowrate of the sensor.
 * 
 * \param fm Flow meter to read rate from.
 * \return Flow rate in volume/s.
 */
float flow_meter_rate(flow_meter fm);

/**
 * \brief Resets the volume and flow rate to 0.
 * 
 * \param fm Flow meter to zero. 
 */
void flow_meter_zero(flow_meter fm);

#endif
/** \} */