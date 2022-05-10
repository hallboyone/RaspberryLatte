import components
import PID
import sys
from time import sleep

#pressure    = pico.get_pressure().in_bar()
#print(pico.get_tempurature().in_C())
#dial_switch = pico.get_switches().dial()
#pump_switch = pico.get_switches().pump()
#weight      = pico.get_weight().in_g()

temp_sensor = components.LMT01()
heater_pwm = components.Heater()
boiler_gains = PID.PIDGains(3, 0.01, 0.1)

boiler_controller = PID.PID(boiler_gains, setpoint = 90, sensor = temp_sensor, output = heater_pwm)

for i in range(120):
    boiler_controller.tick()
    sleep(1)
heater_pwm.off()