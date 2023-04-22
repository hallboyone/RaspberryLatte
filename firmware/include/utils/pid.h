/**
 * \defgroup pid PID Controller Library
 * \ingroup utils
 * \version 0.3
 * 
 * \brief Implementation of a PID controller.
 * 
 * The PID controller is very common in applications such as temperature control. This library
 * provides an easy method of setting one up. 
 * 
 * The PID controller uses callback functions to read the sensor data and update the output. This makes
 * using the PID as easy as passing it to ::pid_tick. This function will only tick if some minimum
 * length of time has passed since the last tick, thereby limiting the update frequency. 
 * 
 * The derivative, if used, is computed as the slope of best fit for all points in some interval. This
 * helps filter noise that could negatively effect performance. Furthermore, the integral contains 
 * windup bounds that automatically clip the error sum at user-defined values.
 * 
 * Changelog:
 * v0.3 - Made structs opaque to ensure proper usage.
 * 
 * v0.2 - Switched from floating point data to integer data. Input is still floating point, however.
 * Dramatically improved discrete derivative calculation speed.
 * 
 * \{
 * 
 * \file
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief PID Library header
 * \date 2022-08-16
 * 
 * The PID controller is defined as a pid_ctrl struct containing the required elements such as the gains
 * (.K), sensor, and plant. The sensor is a pointer to a function that takes no parameters and returns a
 * float while the plant is a pointer to a void function that takes a single float parameter.
 * 
 * \example  examples/pid_ex.c
 * Once every 100ms, this code will call sensor_read_float(), compute the resulting input u, and 
 * then call plant_apply_float(u).
 */

#ifndef PID_H
#define PID_H

#include "pico/stdlib.h"
#include "pico/time.h"
#include "stdint.h"

#define PID_NO_WINDUP_LB INT32_MIN  /**< Small value used to indicate no windup lower bound */
#define PID_NO_WINDUP_UB INT32_MAX  /**< Large value used to indicate no windup upper bound */

/** \brief Sensor measurements in PID library */
typedef float pid_data;
/** \brief Integer type of timestamps in PID library */
typedef uint32_t pid_time;
/** \brief Opaque type defining a discrete derivative with configurable time-window and update frequency */
typedef struct discrete_derivative_s* discrete_derivative;
/** \brief Opaque type defining a discrete integral with configurable windup bounds */
typedef struct discrete_integral_s* discrete_integral;
/** \brief Opaque type defining a PID controller */
typedef struct pid_s* pid;

/** \brief Stuct containing the floating point gains of a PID controller */
typedef struct {
    float p; /**< The proportional gain. */
    float i; /**< The integral gain. */
    float d; /**< The derivative gain. */
    float f; /**< The feedforward gain. */
} pid_gains;

typedef struct {
    float u_p;
    float u_i;
    float u_d;
    float u_ff;
    float u_bias;
} pid_viewer;

/**
 * \brief Typedef of pointer to function taking no parameters but returning float. Used to gather
 * sensor data.
 */
typedef pid_data (*sensor_getter)();

/**
 * \brief Typedef of pointer to function that takes a float and applies it to some plant. Used to
 * apply inputs from controller.
 */
typedef void (*input_setter)(float);

/**
 * \brief Struct containing a data value and the time (in milliseconds since boot) the value was read
 */
typedef struct {
    pid_time t; /**< Time associated with datapoint */
    pid_data v; /**< Value associated with datapoint */
} datapoint;

/** \brief Helper function returning the milliseconds since booting */
pid_time ms_since_boot();

/**
 * \brief Allocate memory for discrete_derivative and initialize.
 * 
 * \param filter_span_ms Slope is computed over all datapoints taken within the last filter_span_ms
 * \param sample_rate_ms The minimum duration between new datapoints.
 * 
 * \returns A new discrete_derivative pointing at an internally managed struct.
 */
discrete_derivative discrete_derivative_setup(const uint filter_span_ms, const uint sample_rate_ms);

/**
 * \brief Resets the discrete_derivative to initial values. Memory is not freed.
 * 
 * \param d The discrete_derivative object that will be reset
 */
void discrete_derivative_reset(discrete_derivative d);

/**
 * \brief Free internal memory and destroy object.
 * 
 * \param d The discrete_derivative object that will be destroyed
 */
void discrete_derivative_deinit(discrete_derivative d);

/**
 * \brief Computes the linear slope of the current datapoints.
 * 
 * \param d The discrete_derivative object that will be read
 * 
 * \returns The slope of the previous datapoints within the filter_span of d. If less 
 * than 2 datapoints, returns 0.
 */
float discrete_derivative_read(discrete_derivative d);

/**
 * \brief Adds a datapoint to the internally managed time series.
 * 
 * Function returns immediately if datapoint is too close to last added datapoint. 
 * Otherwise, the buffer is expanded if needed, the datapoint is added, old datapoints
 * are removed, and the origin is shifted (if needed).
 * 
 * \param d The discrete_derivative object that the point will be added to
 * \param p Datapoint struct with the value and timestamp of the new reading.
 */
void discrete_derivative_add_datapoint(discrete_derivative d, const datapoint p);

/**
 * \brief Adds a value to the internally managed time series as if measured at the current time.
 * 
 * Function returns immediately if datapoint is too close to last added datapoint. 
 * Otherwise, the buffer is expanded if needed, the datapoint is added, old datapoints
 * are removed, and the origin is shifted (if needed).
 * 
 * \param d The discrete_derivative object that the point will be added to
 * \param v Value of a new reading.
 */
void discrete_derivative_add_value(discrete_derivative d, const pid_data v);

/**
 * \brief Print the current internal state of the discrete_derivative.
 * 
 * Internal state of the derivative remains unchanged, unlike when the value
 * is read which removes old points.
 * 
 * \param d The discrete_derivative object to print.
*/
void discrete_derivative_print(const discrete_derivative d);

/**
 * \brief  Allocate memory for discrete_integral and initialize.
 *
 * \param lower_bound Lower bound on the integral's value.
 * \param upper_bound Upper bound on the integral's value.
 * 
 * \returns Newly created discrete_integral object.
 */
discrete_integral discrete_integral_setup(const pid_data lower_bound, const pid_data upper_bound);

/**
 * \brief Helper function that returns the value of the integral's sum field.
 * 
 * \param i The discrete_integral object that will be read.
 *
 * \returns Value of integral: i->sum. 0 if only 0 or 1 point so far.
 */
pid_data discrete_integral_read(const discrete_integral i);

/**
 * \brief Compute the area under the curve since the last time this function was called and add to
 * i's sum field. This area is computed assuming a linear change between the two data points: sum +=
 * 0.5*(prev_val + cur_val)*(cur_t - prev_t). If there was no previous datapoint, then p is saved
 * internally and 0 is returned.
 *
 * \param i The discrete_integral object that will be added to.
 * \param p Datapoint to add to discrete_integral
 */
void discrete_integral_add_datapoint(discrete_integral i, const datapoint p);

/**
 * \brief Update the windup bounds on the integral.
 * 
 * \param i The discrete_integral object that will be updated.
 * \param lower_bound The minimum value returned by discrete_integral_read.
 * \param upper_bound The maximum value returned by discrete_integral_read.
*/
void discrete_integral_set_bounds(discrete_integral i, const pid_data lower_bound, const pid_data upper_bound);

/**
 * \brief Reset integral object. Sum is set to 0 and clear previous datapoint.
 *
 * \param i The discrete_integral object that will be cleared.
 */
void discrete_integral_reset(discrete_integral i);

/**
 * \brief Delete integral object, freeing internal memory;
 *
 * \param i The discrete_integral object that will be deleted.
 */
void discrete_integral_deinit(discrete_integral i);

/**
 * \brief Print the current internal state of the discrete_integral.
 * 
 * \param i The discrete_integral object to print.
*/
void discrete_integral_print(const discrete_integral i);

/**
 * \brief Configure a PID controller. Sets the appropriate parameters in internal sum and slope objects.
 * 
 * \param controller Pointer to pid_ctrl that will be initalized
 * \param K The controller gains
 * \param feedback_sensor Sensor used for feedback control
 * \param feedforward_sensor Sensor used for feedforward control
 * \param plant Plant that the control input is applied to
 * \param u_lb The minimum value the input can have.
 * \param u_ub The maximum value the input can have.
 * \param time_between_ticks_ms Minimum length of time between ticks.
 * \param derivative_filter_span_ms The length of time in ms over which to compute the average slope.
 */
pid pid_setup(const pid_gains K, sensor_getter feedback_sensor, sensor_getter feedforward_sensor, 
              input_setter plant, float u_lb, float u_ub, const uint16_t time_between_ticks_ms, 
              const uint derivative_filter_span_ms);

/**
 * \brief Update the PID controller's setpoint.
 * 
 * \param controller The pid object whose setpoint will be updated.
 * \param setpoint The new setpoint of the pid object
 */
void pid_update_setpoint(pid controller, const pid_data setpoint);

/**
 * \brief Set the controller's internal bias
 * 
 * \param controller The pid object to reset
 * \param bias The bias applied to the controller after reset.
 */
void pid_update_bias(pid controller, float bias);

/**
 * \brief If the minimum time between ticks has elapsed, run one loop of the controller. This requires reading the sensor,
 * updating the sum and slope terms, computing the input, and applying it to the plant (if not NULL). 
 * 
 * \param controller The pid object to update.
 */
float pid_tick(pid controller, pid_viewer * viewer);

/**
 * \brief Checks if the plant is at the target setpoint +/- a tolerance
 * 
 * \param controller The pid object to check.
 * \param tol The range around the setpoint that are considered good.
 * 
 * \return True if at setpoint. False otherwise.
 */
bool pid_at_setpoint(const pid controller, const pid_data tol);

/**
 * \brief Reset the internal fields of the PID controller
 * 
 * \param controller The pid object to reset
 */
void pid_reset(pid controller);

/**
 * \brief Destroy the internal fields of the PID controller (frees memory)
 * 
 * \param controller The pid object to destroy
 */
void pid_deinit(pid controller);
#endif
/** \} */