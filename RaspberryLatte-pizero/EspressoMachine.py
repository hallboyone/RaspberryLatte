import components
from PID import PIDGains, PID, IntegralBounds
from time import sleep

_PWR_OFF     = -1
_STEAM_MODE  =  0
_HOT_MODE    =  1
_MANUAL_MODE =  2
_AUTO_MODE   =  3

class EspressoMachine:
    def __init__(self) -> None:
        self.power_status = components.ACStatus()

        self.heater = components.Heater()
        self.temp_sensor = components.LMT01()
        self.boiler_gains = PIDGains(3, 0.05, 0.25)
        self.boiler_ctrl = PID(self.boiler_gains, sensor=self.temp_sensor, output = self.heater, windup_bounds = IntegralBounds(0, 300))
        self.boiler_setpoints = {"brew":90, "hot":100, "steam":150}

        self.pump = components.Pump()
        self.solenoid = components.Solenoid()

        self.leds = components.LEDs()

        self.switches = components.Switches()

        self.current_mode = _MANUAL_MODE

    def run(self):
        while(True):
            # Check ac switch and dial and update setpoints accordingly 
            self._update_mode()
            
            # Updating boiler
            self.boiler_ctrl.tick()
            self.leds.set(1, self.boiler_ctrl.at_setpoint(2.5))

            # Turn pump off or on depending on mode and pump switch
            self._update_pump()
            sleep(0.01)

    def _update_mode(self):
        if not self.power_status.read():
            print("Machine off")
            self._powered_down_loop()
            print("Machine on")
        else:
            new_mode = self.switches.read("dial")
            if new_mode != self.current_mode:
                self.current_mode = new_mode
                print(new_mode)
                self._update_setpoint

    def _update_pump(self):
        if self.switches.read("pump") and self.current_mode==_MANUAL_MODE:
            self.solenoid.open()
            self.pump.on()
        elif self.switches.read("pump") and self.current_mode==_HOT_MODE:
            self.solenoid.close()
            self.pump.on()
        else:
            self.solenoid.close()
            self.pump.off()

    def _powered_down_loop(self):       
        self.leds.set(0, 0)
        self.heater.off()
        while self.current_mode == _PWR_OFF:
            # Wait until machine powered on 
            self._update_mode()
        self.leds.set(0, 1)
        self.boiler_ctrl.reset()

    def _update_setpoint(self):
        if self.current_mode == _AUTO_MODE or self.current_mode == _MANUAL_MODE:
            self.boiler_ctrl.update_setpoint_to(self.boiler_setpoints["brew"])
        elif self.current_mode == _HOT_MODE:
            self.boiler_ctrl.update_setpoint_to(self.boiler_setpoints["hot"])
        elif self.current_mode == _STEAM_MODE:
            self.boiler_ctrl.update_setpoint_to(self.boiler_setpoints["steam"])

