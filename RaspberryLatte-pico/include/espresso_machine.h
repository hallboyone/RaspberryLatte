#include "machine_configuration.h"
#include "pinout.h"
#include "message_ids.h"

#include "maintainer.h"
#include "uart_bridge.h"

#include "analog_input.h"
#include "binary_output.h"
#include "binary_input.h"
#include "phasecontrol.h"
#include "nau7802.h"
#include "slow_pwm.h"
#include "lmt01.h"

#include "pid.h"
#include "autobrew.h"

analog_input pressure_sensor;
binary_output leds;
binary_input pump_switch, mode_dial;
phasecontrol pump;
binary_output solenoid;
slow_pwm heater;
pid_ctrl heater_pid;
lmt01 thermo;

autobrew_leg autobrew_legs [5];
autobrew_routine autobrew_plan;

/** \brief Helper function for the PID controller. Returns the boiler temp in C. */
float read_boiler_thermo();

/** \brief Helper function for the PID controller. Applies an input to the boiler heater. */
void apply_boiler_input(float u);

/** \brief Returns true if scale is greater than or equal to the passed in value. */
bool scale_at_val(int val_mg);

/** \brief Returns true if scale is greater than or equal to the configured output. */
bool scale_at_output();

void update_setpoint();
void update_pump_lock();
void update_pump();
void update_leds();