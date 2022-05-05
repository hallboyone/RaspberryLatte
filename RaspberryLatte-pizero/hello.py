import uart_bridge as from_pico

pressure = from_pico.getPressure().in_bar()
temp = from_pico.getTempurature().in_C()
dial = from_pico.getSwitches().dial()