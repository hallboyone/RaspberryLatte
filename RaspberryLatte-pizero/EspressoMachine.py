import components
from brew_config import *
from PID import PIDGains, PID, IntegralBounds
import AutoBrewScheduler
from time import sleep

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
        self.boiler_setpoints = {"brew":BREW_TEMP, "hot":HOT_TEMP, "steam":STEAM_TEMP}
        self.boiler_ctrl.update_setpoint_to(self.boiler_setpoints["brew"])

        self.pump = components.Pump()
        self.solenoid = components.Solenoid()

        self.leds = components.LEDs()

        self.switches = components.Switches()
        self.switch_state = {"dial": _MANUAL_MODE, "pump": False}

        self.scale = components.Scale()

        self._auto_brew_routine = [
            AutoBrewScheduler.Ramp(from_pwr = 60, to_pwr = PRE_ON_PWR*0.67+60, in_sec = PRE_ON_TIME),
            AutoBrewScheduler.ConstantTimed(pwr = 0,  for_sec = PRE_OFF_TIME),
            AutoBrewScheduler.Ramp(from_pwr = 60, to_pwr = 127, in_sec = 1),
            AutoBrewScheduler.ConstantTriggered(lambda : self.scale.read() >= YIELD, pwr = 127)]
        self._auto_brew_schedule = AutoBrewScheduler.AutoBrewScheduler(self._auto_brew_routine)

    def run(self):
        while(True):
            if not self.power_status.read():
                self._powered_down_loop()
            
            # Check pump switch and dial. Update object accordingly 
            self._update_mode()
            
            # Updating boiler and turn on LED if at setpoint
            self.boiler_ctrl.tick()
            self.leds.set(1, self.boiler_ctrl.at_setpoint(2.5))

            # Turn pump off or on depending on mode and pump switch
            self._update_pump()

            sleep(0.01)

    def _update_mode(self):
        new_switch_state = self.switches.read()
        if self.switch_state["dial"] != new_switch_state["dial"]:
            self.switch_state["dial"] = new_switch_state["dial"] 
            self._update_setpoint()
            if (new_switch_state["dial"] == _AUTO_MODE):
                self._auto_brew_schedule.reset()
        
        if not new_switch_state["pump"]:
            self._auto_brew_schedule.reset()
            self.switch_state["pump"] = False
        else:
            self.switch_state["pump"] = True

    def _update_pump(self):
        if self.switch_state   == {"pump": True, "dial": _MANUAL_MODE}:
            self.solenoid.open()
            self.pump.on()
        elif self.switch_state == {"pump": True, "dial": _HOT_MODE   }:
            self.solenoid.close()
            self.pump.on()
        elif self.switch_state == {"pump": True, "dial": _AUTO_MODE  }:
            val, val_changed, finished = self._auto_brew_schedule.tick()
            if not finished:
                self.solenoid.open()
                if val_changed:
                    print(f"Setting pump to {val}")
                    self.pump.set(val)
            else:
                self.pump.off()
                self.solenoid.close()
        else:
            self.solenoid.close()
            self.pump.off()

    def _powered_down_loop(self): 
        print("Machine off")      
        self.leds.set_all(0,0,0)
        self.heater.off()
        self.solenoid.close()
        self.pump.off()
        while not self.power_status.read():
            pass
        self.leds.set(0, 1)
        self.boiler_ctrl.reset()
        self._auto_brew_schedule.reset()
        print("Machine on")

    def _update_setpoint(self):
        if self.switch_state["dial"] == _AUTO_MODE or self.switch_state["dial"] == _MANUAL_MODE:
            self.boiler_ctrl.update_setpoint_to(self.boiler_setpoints["brew"])
        elif self.switch_state["dial"] == _HOT_MODE:
            self.boiler_ctrl.update_setpoint_to(self.boiler_setpoints["hot"])
        elif self.switch_state["dial"] == _STEAM_MODE:
            self.boiler_ctrl.update_setpoint_to(self.boiler_setpoints["steam"])