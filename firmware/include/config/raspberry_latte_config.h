// Between two temp readings, the temperature must not change by more than this amount.
#define THERMAL_RUNAWAY_WATCHER_MAX_CONSECUTIVE_TEMP_CHANGE_16C 10*16

// Defines the window over which a change in temperature must be observed while converging to setpoint.
#define THERMAL_RUNAWAY_WATCHER_MIN_TEMP_CHANGE_PERIOD_MS 20000
#define THERMAL_RUNAWAY_WATCHER_MIN_TEMP_CHANGE_HEAT_16C 2*16
#define THERMAL_RUNAWAY_WATCHER_MIN_TEMP_CHANGE_COOL_16C 1*16

// Defines how close to the setpoint counts as convergence and how far counts as divergence
#define THERMAL_RUNAWAY_WATCHER_CONVERGENCE_TOL_16C 1*16
#define THERMAL_RUNAWAY_WATCHER_DIVERGENCE_TOL_16C 8*16