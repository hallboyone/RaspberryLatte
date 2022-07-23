#include "pico/stdlib.h"

typedef struct datapoint_{
    uint64_t t;
    float v;
} datapoint;

typedef struct discrete_derivative_{
    uint filter_span_ms;
    datapoint * _data;
    uint16_t _buf_len;
    uint16_t _num_el;
} discrete_derivative;

/**
 * \brief Clear the internal fields of struct d and initalize.
 */
void discrete_derivative_init(discrete_derivative * d, uint filter_span_ms);

/**
 * \brief Release internal memory and reset object.
 */
void discrete_derivative_deinit(discrete_derivative * d);

/**
 * \brief Computes the linear slope of the current datapoints.
 * \returns The slope of the previous datapoints withint the filter_span of d. If only
 * 0 or 1 point, returns 0.
 */
float discrete_derivative_read(discrete_derivative * d);

/**
 * \brief Updates the internally managed time series and computes its linear slope. 
 * \returns The slope of the previous datapoints withint the filter_span of d. If only
 * 1 point, returns 0.
 */
float discrete_derivative_add_point(discrete_derivative * d, datapoint p);

/**
 * \brief Resets the discrete_derivative to initial values. Memory is not freed.
 */
void discrete_derivative_clear(discrete_derivative * d);

typedef struct discrete_integral_{
    float sum;
    float lower_bound;
    float upper_bound;
    datapoint prev_p;
} discrete_integral;

void discrete_integral_init(discrete_integral * d, const float * lower_bound, const float * upper_bound);

float discrete_integral_read(discrete_integral * i);

float discrete_integral_add_point(discrete_integral * i, datapoint p);

void discrete_integral_clear(discrete_integral * i);


typedef struct pid_gains_{
    float p;
    float i;
    float d;
} pid_gains;

typedef float (*sensor_reader)();

typedef struct pid_ctrl_{
    pid_gains K;
    sensor_reader s;
    float setpoint;

} pid_ctrl;