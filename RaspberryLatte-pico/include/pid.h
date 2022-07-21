#include "pico/stdlib.h"

typedef struct datapoint_{
    uint32_t t;
    float d;
} datapoint;

typedef struct discrete_derivative_{
    uint filter_span_ms;
    datapoint * data;
} discrete_derivative;

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

/**
 * \brief Clear the internal fields of struct d and initalize.
 */
void discrete_derivative_init(discrete_derivative * d, uint filter_span_ms);

/**
 * \brief Updates the internally managed time series and computes its linear slope. 
 * \returns The slope of the previous datapoints withint the filter_span of d. If only
 * 1 point, returns 0.
 */
float discrete_derivative_add_point(discrete_derivative * d, datapoint p);

/**
 * \brief Computes the linear slope of the current datapoints.
 * \returns The slope of the previous datapoints withint the filter_span of d. If only
 * 0 or 1 point, returns 0.
 */
float discrete_derivative_read(discrete_derivative * d);