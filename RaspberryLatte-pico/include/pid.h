#include "pico/stdlib.h"

typedef struct datapoint_ {
    uint64_t t;
    float v;
} datapoint;

typedef struct discrete_derivative_ {
    uint filter_span_ms;
    datapoint* _data;
    uint16_t _buf_len;
    uint16_t _num_el;
} discrete_derivative;

/**
 * \brief Clear the internal fields of struct d and initalize.
 */
void discrete_derivative_init(discrete_derivative* d, uint filter_span_ms);

/**
 * \brief Release internal memory and reset object.
 */
void discrete_derivative_deinit(discrete_derivative* d);

/**
 * \brief Computes the linear slope of the current datapoints.
 * \returns The slope of the previous datapoints withint the filter_span of d. If only
 * 0 or 1 point, returns 0.
 */
float discrete_derivative_read(discrete_derivative* d);

/**
 * \brief Updates the internally managed time series and computes its linear slope.
 * \returns The slope of the previous datapoints withint the filter_span of d. If only
 * 1 point, returns 0.
 */
float discrete_derivative_add_point(discrete_derivative* d, datapoint p);

/**
 * \brief Resets the discrete_derivative to initial values. Memory is not freed.
 */
void discrete_derivative_clear(discrete_derivative* d);

typedef struct discrete_integral_ {
    float sum;
    float lower_bound;
    float upper_bound;
    datapoint prev_p;
} discrete_integral;

/**
 * \brief Clear the internal parameters of the discrete_integral d and set the lower and upper
 * bounds.
 *
 * \param d Pointer to discrete_integral stuct that will be initalized.
 * \param lower_bound Pointer to a floating point lower bound on the integral's value. If NULL, set
 * to -10000. 
 * \param upper_bound Pointer to a floating point upper bound on the integral's value. If NULL, set to 10000.
 */
void discrete_integral_init(discrete_integral* d, const float* lower_bound,
                            const float* upper_bound);

/**
 * \brief Helper function that returns the value of the integral's sum field.
 * \param i Pointer to discrete_integral stuct that will be read.
 *
 * \returns Value of integral: i->sum.
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
 *
 * \returns The first time the function is called on i after initalizing or clearing it, 0 is
 * returned. After the, the value of integral after adding the datapoint p is returned.
 */
float discrete_integral_add_point(discrete_integral* i, datapoint p);

/**
 * \brief Reset integral object. Sum is set to 0 and clear previous datapoint.
 *
 * \param i Pointer to discrete_integral stuct that will be cleared.
 */
void discrete_integral_clear(discrete_integral* i);

typedef struct pid_gains_ {
    float p;
    float i;
    float d;
} pid_gains;
/**
 * \brief Typedef of pointer to function taking no parameters but returning float. Used to gather
 * sensor data.
 */
typedef float (*read_sensor)();
/**
 * \brief Typedef of pointer to function that takes a float and applies it to some plant. Used to
 * apply inputs from controller.
 */
typedef void (*apply_input)(float);

typedef struct pid_ctrl_ {
    pid_gains K;
    float setpoint;

    read_sensor sensor;
    apply_input plant;
} pid_ctrl;