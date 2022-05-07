import uart_bridge as pico
import sys

#pressure    = pico.get_pressure().in_bar()
#print(pico.get_tempurature().in_C())
#dial_switch = pico.get_switches().dial()
#pump_switch = pico.get_switches().pump()
#weight      = pico.get_weight().in_g()

pico.set_solenoid_to(int(sys.argv[1]))
print(f"Set to {int(sys.argv[1])}")