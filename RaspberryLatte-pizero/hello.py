import uart_bridge as pico

pressure    = pico.get_pressure().in_bar()
temp        = pico.get_tempurature().in_C()
dial_switch = pico.get_switches().dial()
pump_switch = pico.get_switches().pump()
weight      = pico.get_weight().in_g()

set_heater_to(50)