import components
from PID import PIDGains, PID
from time import sleep

class EspressoMachine:
    _STEAM_MODE  = 0
    _HOT_MODE    = 1
    _MANUAL_MODE = 2
    _AUTO_MODE   = 3

    power_status = components.ACStatus()

    heater = components.Heater()
    temp_sensor = components.LMT01()
    boiler_gains = PIDGains(3, 0.01, 0.1)
    boiler_ctrl = PID(boiler_gains, sensor=temp_sensor, output = heater)

    pump = components.Pump()
    solenoid = components.Solenoid()

    leds = components.LEDs()

    switches = components.Switches()

    def _update_pump(self):
        # Pump only comes on if
        # (a) In manual mode and pump switch is on
        # (b) TO-DO In auto mode, pump switch is on, and ready to brew
        # (c) In h0t-water mode and pump switch is on
        if self.switches.read("pump"):
            if self.switches.read("mode")==self._MANUAL_MODE:
                self.solenoid.set(1)
                self.pump.set(127)
            elif self.switches.read("mode")==self._HOT_MODE:
                self.solenoid.set(0)
                self.pump.set(127)
            else:
                self.solenoid.set(0)
                self.pump.set(0)
        else:
            self.solenoid.set(0)
            self.pump.set(0)

    def run(self):
        self.boiler_ctrl.update_setpoint_to(90)
        while(True):
            if not self.power_status.read():
                self.leds.set(0, 0)
                self.heater.off()
                while not self.power_status.read():
                    pass
            self.leds.set(0, 1)
            # (1) TO-DO Check for user input
            # (2) Update Heater
            # (3) Update Pump
            # (4) Update LEDs

            self.boiler_ctrl.tick()
            self._update_pump()
            sleep(0.01)



