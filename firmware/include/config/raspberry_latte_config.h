/**
 * Length of time after powering on that the boiler and pump remain inactive. 
 * Let's transient voltages settle out before activating the machine.
 */
#define AC_SETTLING_TIME_MS 500

#define BOILER_PID_GAIN_P 0.05
#define BOILER_PID_GAIN_I 0.00000175
#define BOILER_PID_GAIN_D 0.0
#define BOILER_PID_GAIN_F 0.00005
#define FLOW_PID_GAIN_P   0.0125
#define FLOW_PID_GAIN_I   0.00004
#define FLOW_PID_GAIN_D   0.0
#define FLOW_PID_GAIN_F   0.0

#define SCALE_CONVERSION_MG -0.152710615479

/** ml per pulse of pump flow sensor. */
#define PULSE_TO_FLOW_CONVERSION_ML  0.5 

// Between two temp readings, the temperature must not change by more than this amount.
#define THERMAL_RUNAWAY_WATCHER_MAX_CONSECUTIVE_TEMP_CHANGE_cC 1000

// Defines the window over which a change in temperature must be observed while converging to setpoint.
#define THERMAL_RUNAWAY_WATCHER_MIN_TEMP_CHANGE_PERIOD_MS 20000
#define THERMAL_RUNAWAY_WATCHER_MIN_TEMP_CHANGE_HEAT_cC  200
#define THERMAL_RUNAWAY_WATCHER_MIN_TEMP_CHANGE_COOL_cC  100

// Defines how close to the setpoint counts as convergence and how far counts as divergence
#define THERMAL_RUNAWAY_WATCHER_CONVERGENCE_TOL_cC 100
#define THERMAL_RUNAWAY_WATCHER_DIVERGENCE_TOL_cC  800

#define PUMP_SWITCH_DEBOUNCE_DURATION_US 10000
#define MODE_DIAL_DEBOUNCE_DURATION_US 75000

#define AUTOBREW_PREINF_END_PRESSURE_MBAR 3000
#define AUTOBREW_BREW_PRESSURE_MBAR       9000

#define THERMAL_RUNAWAY_WATCHER_LED_TOGGLE_PERIOD_MS 512

/**
 * The temp that the thermometer is reading may not match the water at the group-head.
 * This constant defines the discrepancy. Compute the value in C and then multiply by 
 * 16 and round to an integer. 
*/
#define BOILER_TEMP_OFFSET_cC 800