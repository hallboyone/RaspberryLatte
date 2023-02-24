/**
 * \defgroup pid PID Controller Library
 * \ingroup utils
 * \version 0.2
 * 
 * \brief Implementation of a PID controller.
 * 
 * The PID controller is very common in applications such as temperature control. This library
 * provides an easy method of setting one up. Unlike most other libraries in RaspberryLatte, you use
 * the PID controller by configuring its settings in a struct and then passing this struct to the
 * ::pid_init function. This will change in the future (see To-Do below). 
 * 
 * The PID controller uses callback functions to read the sensor data and update the output. This makes
 * using the setup PID as easy as passing it to ::pid_tick. This function will only tick if some minimum
 * length of time has passed since the last tick, thereby limiting the update frequency. 
 * 
 * The derivative, if used, is computed as the slope of best fit for all points in some interval. This
 * helps filter noise that could negatively effect performance. Furthermore, the integral contains 
 * windup bounds that automatically clip the error sum at user-defined values.
 * 
 * 
 * \todo make consistent with rest of RaspberryLatte's firmware. 
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

#define PID_NO_WINDUP_LB -1000000 /**< Small value used to indicate no windup lower bound */
#define PID_NO_WINDUP_UB  1000000 /**< Large value used to indicate no windup upper bound */

/**
 * \brief Helper function returning the microseconds since booting
 */
float sec_since_boot();
uint64_t ms_since_boot();

typedef int32_t pid_data_t;
typedef int64_t pid_time_t;

/**
 * \brief Struct containing a floating point value and the time (in seconds since boot) the value was read
 */
typedef struct {
    pid_time_t t; /**< Time associated with datapoint */
    pid_data_t v; /**< Value associated with datapoint */
} datapoint;

/**
 * \brief Stuct representing a discrete derivative.
 * 
 * A discrete derivative is essentially the slope of best fit for the sequence of most recent datapoints
 * in a time series. All points measured within some number of ms are used to create this slope of best
 * fit.
 */
typedef struct {
    uint filter_span_ms;        /**< The amount of the data series in ms that the slope will be fitted to. */
    uint ms_between_datapoints;
    datapoint * _data;          /**< The most recent datapoints in the dataseries. */
    uint16_t _buf_len;          /**< The max number of datapoints the _data can hold. */
    uint16_t _num_el;           /**< Number of datapoints in buffer. */
    uint16_t _start_idx;        /**< The first index of the data in array (inclusive). */
    datapoint _origin;
    int64_t _sum_v;
    int64_t _sum_t;
    int64_t _sum_vt;
    int64_t _sum_tt;
} discrete_derivative;

/**
 * \brief Clear the internal fields of struct d and initalize.
 * 
 * \param d Pointer to discrete_derivative that will be initalized
 * \param filter_span_ms Slope is computed over all datapoints taken within the last filter_span_ms
 */
void discrete_derivative_setup(discrete_derivative* d, uint filter_span_ms, uint ms_between_datapoints);

/**
 * \brief Release internal memory and reset object.
 * 
 * \param d Pointer to discrete_derivative that will be destroyed
 */
void discrete_derivative_deinit(discrete_derivative* d);

/**
 * \brief Computes the linear slope of the current datapoints.
 * 
 * \param d Pointer to discrete_derivative that will be read
 * 
 * \returns The slope of the previous datapoints within the filter_span of d. If only
 * 0 or 1 point, returns 0.
 */
float discrete_derivative_read(discrete_derivative* d);

/**
 * \brief Updates the internally managed time series.
 * 
 * \param d Pointer to discrete_derivative that the point will be added to
 * \param p Datapoint struct with the value and timestamp of the new reading.
 */
void discrete_derivative_add_datapoint(discrete_derivative* d, datapoint p);

/**
 * \brief Updates the internally managed time series.
 * 
 * \param d Pointer to discrete_derivative that the point will be added to
 * \param v Value of a new reading.
 */
void discrete_derivative_add_value(discrete_derivative* d, int64_t v);

/**
 * \brief Resets the discrete_derivative to initial values. Memory is not freed.
 */
void discrete_derivative_reset(discrete_derivative* d);

/**
 * \brief Stuct representing a discrete integral.
 * 
 * A discrete integral is the area under the curve of a series of datapoints. The area is computed
 * by averaging the value between current and previous datapoint and multiplying that by the duration
 * between the two datapoints. An effect of this math is the area under the curve is 0 until two data
 * points have been passed to it.
 */
typedef struct {
    float  sum;         /**< The current area under the curve. */
    pid_data_t lower_bound; /**< The lower bound on the area under the curve. Useful for antiwindup. */
    pid_data_t upper_bound; /**< The upper bound on the area under the curve. Useful for antiwindup. */
    datapoint prev_p;  /**< The previous datapoint. Updated each time the integral is ticked. */
} discrete_integral;

/**
 * \brief Clear the internal parameters of the discrete_integral d and set the lower and upper
 * bounds.
 *
 * \param i Pointer to discrete_integral stuct that will be initalized.
 * \param lower_bound Lower bound on the integral's value.
 * \param upper_bound Upper bound on the integral's value.
 */
void discrete_integral_setup(discrete_integral* i, const pid_data_t lower_bound,
                            const pid_data_t upper_bound);

/**
 * \brief Helper function that returns the value of the integral's sum field.
 * \param i Pointer to discrete_integral stuct that will be read.
 *
 * \returns Value of integral: i->sum. 0 if only 0 or 1 point so far.
 */
float discrete_integral_read(discrete_integral* i);

/**
 * \brief Compute the area under the curve since the last time this function was called and add to
 * i's sum field. This area is computed assuming a linear change between the two data points: sum +=
 * 0.5*(prev_val + cur_val)*(cur_t - prev_t). If there was no previous datapoint, then p is saved
 * internally and 0 is returned.
 *
 * \param i Pointer to discrete_integral stuct that will be added to.
 * \param p Datapoint to add to discrete_integral
 */
void discrete_integral_add_datapoint(discrete_integral* i, datapoint p);

/**
 * \brief Reset integral object. Sum is set to 0 and clear previous datapoint.
 *
 * \param i Pointer to discrete_integral stuct that will be cleared.
 */
void discrete_integral_reset(discrete_integral* i);

/**
 * \brief Stuct containing the floating point gains of a PID controller
 */
typedef struct {
    float p; /**< The proportional gain. */
    float i; /**< The integral gain. */
    float d; /**< The derivative gain. */
    float f; /**< The feedforward gain. */
} pid_gains;

/**
 * \brief Typedef of pointer to function taking no parameters but returning float. Used to gather
 * sensor data.
 */
typedef pid_data_t (*read_sensor)();

/**
 * \brief Typedef of pointer to function that takes a float and applies it to some plant. Used to
 * apply inputs from controller.
 */
typedef void (*apply_input)(float);

/**
 * \brief Struct defining a PID object. The setpoint, K, sensor, plant, and min_time_between_ticks_ms must be set explicitly. 
 * The others are set by pid_init.
 */
typedef struct {
    pid_data_t setpoint;                     /**< The current setpoint the PID is regulating to. */
    pid_gains K;                        /**< The gains of the PID controller. */
    read_sensor sensor;                 /**< The sensor function. */
    read_sensor sensor_feedforward;     /**< The sensor providing feedforward values. */
    apply_input plant;                  /**< The plant function that applies the input to the system. */
    discrete_derivative err_slope;      /**< A discrete_derivative tracking the slope of the error. */
    discrete_integral err_sum;          /**< A discrete_integral tracking the sum of the error. */
    uint16_t min_time_between_ticks_ms; /**< The minimum time, in ms, between ticks. If time hasn't elapsed, the previous input is returned. */
    absolute_time_t _next_tick_time;    /**< Timestamp used to track the earliest time that the system can be ticked again. */
} pid_ctrl;

/**
 * \brief Configure a PID controller. Sets the appropriate parameters in internal sum and slope objects.
 * 
 * \param controller Pointer to pid_ctrl that will be initalized
 * \param K The controller gains
 * \param feedback_sensor Sensor used for feedback control
 * \param feedforward_sensor Sensor used for feedforward control
 * \param plant Plant that the control input is applied to
 * \param time_between_ticks_ms Minimum length of time between ticks.
 * \param windup_lb The minimum value the error sum can have.
 * \param windup_ub The maximum value the error sum can have.
 * \param derivative_filter_span_ms The length of time in ms over which to compute the average slope.
 */
void pid_setup(pid_ctrl * controller, const pid_gains K, read_sensor feedback_sensor,
               read_sensor feedforward_sensor, apply_input plant, uint16_t time_between_ticks_ms,
               const pid_data_t windup_lb, const pid_data_t windup_ub, uint derivative_filter_span_ms);

/**
 * \brief If the minimum time between ticks has elapsed, run one loop of the controller. This requires reading the sensor,
 * updating the sum and slope terms, computing the input, and applying it to the plant (if not NULL). 
 * 
 * \param controller Pointer to PID controller object to update.
 */
float pid_tick(pid_ctrl * controller);

/**
 * \brief Checks if the plant is at the target setpoint +/- a tolerance
 * 
 * \param controller Pointer to PID controller object to check.
 * \param tol The range around the setpoint that are considered good.
 * 
 * \return True if at setpoint. False otherwise.
 */
bool pid_at_setpoint(pid_ctrl * controller, float tol);

/**
 * \brief Reset the internal fields of the PID controller
 */
void pid_reset(pid_ctrl * controller);

/**
 * \brief Destroy the internal fields of the PID controller (frees memory)
 */
void pid_deinit(pid_ctrl * controller);
#endif
/** \} */