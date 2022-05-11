import components
from PID import PIDGains, PID
from time import sleep

_PWR_OFF     = -1
_STEAM_MODE  =  0
_HOT_MODE    =  1
_MANUAL_MODE =  2
_AUTO_MODE   =  3

class EspressoMachine:
    power_status = components.ACStatus()

    heater = components.Heater()
    temp_sensor = components.LMT01()
    boiler_gains = PIDGains(3, 0.01, 0.1)
    boiler_ctrl = PID(boiler_gains, sensor=temp_sensor, output = heater)
    boiler_setpoints = {"brew":90, "hot":100, "steam":150}

    pump = components.Pump()
    solenoid = components.Solenoid()

    leds = components.LEDs()

    switches = components.Switches()

    current_mode = _MANUAL_MODE

    def _update_mode(self) -> bool:
        if not self.power_status.read():
            if current_mode != _PWR_OFF:
                current_mode = _PWR_OFF
                return True
            else:
                return False
        else:
            new_mode = self.switches.read()
            if new_mode != current_mode:
                current_mode = new_mode
                return True
            else:
                return False

    def _update_pump(self):
        # Pump only comes on if
        # (a) In manual mode and pump switch is on
        # (b) TO-DO In auto mode, pump switch is on, and ready to brew
        # (c) In h0t-water mode and pump switch is on
        if self.switches.read("pump") and self.current_mode==_MANUAL_MODE:
            self.solenoid.open()
            self.pump.on()
        elif self.switches.read("pump") and self.current_mode==_HOT_MODE:
            self.solenoid.close()
            self.pump.on()
        else:
            self.solenoid.close()
            self.pump.off()

    def update_to_new_mode(self):
        """
        (1) Update setpoint
        """
        if self.current_mode == _AUTO_MODE or self.current_mode == _MANUAL_MODE:
            self.boiler_ctrl.update_setpoint_to(self.boiler_setpoints["brew"])
        elif self.current_mode == _HOT_MODE:
            self.boiler_ctrl.update_setpoint_to(self.boiler_setpoints["hot"])
        elif self.current_mode == _STEAM_MODE:
            self.boiler_ctrl.update_setpoint_to(self.boiler_setpoints["steam"])

    def _power_down_loop(self):
        self.leds.set(0, 0)
        self.heater.off()
        while self.current_mode == _PWR_OFF:
            # Wait until machine powered on 
            self._update_mode()
        self.leds.set(0, 1)
        self.boiler_ctrl.reset()
        
    def run(self):
        while(True):
            mode_changed = self._update_mode()
            if self.current_mode == _PWR_OFF:
                self.leds.set(0, 0)
                self.heater.off()
                while not self._update_mode():
                    # Wait until machine powered on 
                    pass
                self.leds.set(0, 1)
            
            if mode_changed:
                self._update_to_new_mode()
            
            self.boiler_ctrl.tick()
            self._update_pump()
            sleep(0.01)



