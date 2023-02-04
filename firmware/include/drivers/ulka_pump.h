/**
 * \defgroup ulka_pump Ulka Pump Library
 * \version 0.1
 * 
 * \brief Abstracts the use of Ulka vibratory pump. Writes percentage power to the pump. If a
 * flow_meter is configured, then the current flow and current pressure can be computed. Note
 * that pressure is computed from flow and may be inaccurate.
 * 
 * \ingroup drivers
 * @{
 * \file
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Header for Ulka Pump Library
 * \version 0.1
 * \date 2023-01-30
 */

#ifndef ULKA_PUMP_H
#define ULKA_PUMP_H
#include "pico/stdlib.h"
#include "utils/phasecontrol.h"
#include "drivers/flow_meter.h"

typedef struct {
    phasecontrol pump_driver;
    flow_meter pump_flow_meter;
    bool locked;
    float max_pressure;
    uint8_t percent_power;
} ulka_pump;

/**
 * \brief Setup a Ulka pump.
 * 
 * The pump is controlled using a phasecontrol object. These need several pins to work which are passed
 * as parameters to this function.
 * 
 * \param p The pump object to configure
 * \param zerocross_pin The pin attached to the zero-cross circuit. A zero-cross is indicated when the pin
 * experiences a \p zerocross_event defined below.
 * \param out_pin The output pin controlling when the pump receives power.
 * \param zerocross_shift_us The length of time between the actually zero-cross and when it is sensed.
 * \param zerocross_event The event (rising or falling) that signals a zero-cross
 * \return PICO_ERROR_NONE on success. Else and error code is returned.
 */
int ulka_pump_setup(ulka_pump * p, uint8_t zerocross_pin, uint8_t out_pin, int32_t zerocross_shift_us, uint8_t zerocross_event);

/**
 * \brief Configure an internal flow meter for the pump.
 * 
 * This allows the pumps flow rate and pressure to be measured.
 * 
 * \param p Previously setup pump that the flow meter will be associated with.
 * \param pin_num The digital pin attached to the flow meter
 * \param conversion The conversion factor between ticks and volume for the flowmeter
 * \return PICO_ERROR_NONE on success. Else and error code is returned. 
 */
int ulka_pump_setup_flow_meter(ulka_pump * p, uint8_t pin_num, float conversion);

/**
 * \brief Set the pumps percent power.
 * 
 * If the pump is locked, this is ignored and 0% power is applied.
 * 
 * \param p A previously setup pump struct to write the power to
 * \param percent_power A percent power between 0 and 100 inclusive. Values out or range are clipped.
 * \return int 
 */
int ulka_pump_pwr_percent(ulka_pump * p, uint8_t percent_power);

/**
 * \brief Turns the pump off.
 * 
 * Equivalent to \ref ulka_pump_pwr_percent(&p, 0);
 * 
 * \param p A previously setup pump struct to write the power to
 */
void ulka_pump_off(ulka_pump * p);

/**
 * \brief Lock the pump and set power to 0
 * 
 * \param p A previously setup pump struct to lock
 */
void ulka_pump_lock(ulka_pump * p);

/**
 * \brief Unlock the pump.
 * 
 * \param p A previously setup pump struct to unlock
 */
void ulka_pump_unlock(ulka_pump * p);

/**
 * \brief Return the current percent power
 * 
 * \param p A previously setup pump struct to lock
 * \return The current power from 0 (off) to 100 (full on)
 */
uint8_t ulka_pump_get_pwr(ulka_pump * p);

/**
 * \brief Return the flowrate of the pump.
 * 
 * Requires configuring an internal flow sensor using \ref ulka_pump_setup_flow_meter.
 * 
 * \param p A previously setup pump struct with a configured flow meter.
 * \return the current flow rate if configured. Else 0.
 */
float ulka_pump_get_flow(ulka_pump * p);

/**
 * \brief Return the pressure of the pump.
 * 
 * Vibratory pumps have a relationship between the flow, power, and pressure. Using an 
 * internal flow sensor, the function computes the theoretical pressure.
 * 
 * \param p A previously setup pump struct with a configured flow meter.
 * \return the current pressure if the flow meter has been configured. Else 0.
 */
float ulka_pump_get_pressure(ulka_pump * p);

/**
 * \brief Check if pump is locked.
 * 
 * \param p Pump structure
 * \return True if pump is locked. Else false.
 */
bool ulka_pump_is_locked(ulka_pump * p);

#endif
/** @} */