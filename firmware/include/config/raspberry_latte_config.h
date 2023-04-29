// While heating to a setpoint, the machine must change by the target amount 
// within the period duration. If it doesn't the heater will be disabled
#define THERMAL_RUNAWAY_WATCHER_TEMP_CONVERGENCE_PERIOD_S 10
#define THERMAL_RUNAWAY_WATCHER_TEMP_CONVERGENCE_AMOUNT_C 2

// After settling to a setpoint, the machine will disable the heater if temperature 
// varies by more than this amount
#define THERMAL_RUNAWAY_WATCHER_TEMP_HYSTERESIS_LIMIT_C 8 