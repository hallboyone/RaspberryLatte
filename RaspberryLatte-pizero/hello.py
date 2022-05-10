import components
import PID
import sys
from time import sleep

# Getters
temp_sensor     = components.LMT01()
pressure_sensor = components.PressureSensor()
weight_sensor   = components.Scale()
switches        = components.Switches()
ac_status       = components.ACStatus()

# Setters
heater_pwm      = components.Heater()

# Build and run PID controller
boiler_gains = PID.PIDGains(3, 0.01, 0.1)
boiler_controller = PID.PID(boiler_gains, setpoint = 90, sensor = temp_sensor, output = heater_pwm)
for i in range(120):
    boiler_controller.tick()
    sleep(1)
heater_pwm.off()