from time import sleep
import configparser

# Getters
from Components.ACSensor       import ACSensor
from Components.PressureSensor import PressureSensor
from Components.Scale          import Scale
from Components.Switches       import Switches
from Components.TempSensor     import TempSensor

# Setters
from Components.Heater         import Heater
from Components.LEDs           import LEDs
from Components.Pump           import Pump
from Components.Solenoid       import Solenoid

from PID import PIDGains, PID, IntegralBounds
import AutoBrewScheduler
import Logger

_STEAM_MODE  =  0
_HOT_MODE    =  1
_MANUAL_MODE =  2
_AUTO_MODE   =  3

class EspressoMachine:
    def __init__(self) -> None:
        self._config = configparser.ConfigParser()
        self._config.read(''.join([__file__[0:__file__.rfind('/')+1], "brew_config"]))

        # Getters 
        self.ac_power     = ACSensor()
        self.temp_sensor  = TempSensor()
        self.switches     = Switches()
        self.scale        = Scale()
        self.pressure     = PressureSensor()

        # Setters
        self.heater       = Heater()
        self.pump         = Pump()
        self.solenoid     = Solenoid()
        self.leds         = LEDs()

        # Controllers
        self.boiler_gains = PIDGains(0.3, 0.005, 0.025)
        self.boiler_ctrl = PID(self.boiler_gains, sensor=self.temp_sensor, output = self.heater, windup_bounds = IntegralBounds(0, 300))
        self.boiler_ctrl.update_setpoint_to(self._config["temps"]["brew"])

        self._logger = Logger.Logger(sample_time=0.05)
        self._logger.add_source("temp", self.temp_sensor.read)
        self._logger.add_source("scale", self.scale.read)
        self._logger.add_source("pressure", self.pressure.read)
        self._logger.add_source("pump", lambda : self.pump._last_setting.val)
        self._logger.add_source("heater", lambda : self.heater._last_setting.val)

        self._auto_brew_routine = [
            AutoBrewScheduler.FunctionCall(self.scale.zero),
            AutoBrewScheduler.Ramp(from_pwr = 60, 
                                   to_pwr = self._config["autobrew"]["PRE_ON_PWR"]*0.67+60, 
                                   in_sec = self._config["temps"]["PRE_ON_TIME"]),
            AutoBrewScheduler.ConstantTimed(pwr = 0,  for_sec = self._config["autobrew"]["PRE_OFF_TIME"]),
            AutoBrewScheduler.Ramp(from_pwr = 60, to_pwr = 127, in_sec = 1),
            AutoBrewScheduler.ConstantTriggered(pwr = 127, trigger_callback = lambda : self.scale.read('g') >= self._config["autobrew"]["YIELD"])]
        self._auto_brew_schedule = AutoBrewScheduler.AutoBrewScheduler(self._auto_brew_routine, logger = self._logger)

    def run(self):
        try:
            while(True):
                if not self.ac_power.on():
                    self._powered_down_loop()
                
                # Check pump switch and dial. Update object accordingly 
                self._update_mode()
                
                # Updating boiler and turn on LED if at setpoint
                self.boiler_ctrl.tick()
                self.leds.set(1, self.boiler_ctrl.at_setpoint(2.5))

                # Turn pump off or on depending on mode and pump switch
                self._update_pump()

                sleep(0.01)
        except:
            print("Shutting down")
            self.leds.set_all(0,0,0)
            self.heater.off()
            self.solenoid.close()
            self.pump.off()

    def _update_mode(self):
        self.switches.read()
        # If dial changed value, update the setpoint and, if switched to auto mode, reset
        # auto brew object
        if self.switches.did_change['dial']:
            self.switches.did_change['dial'] = False
            self._update_setpoint()
            if (self.switches.state('dial') == _AUTO_MODE):
                self._auto_brew_schedule.reset()
        
        if self.switches.did_change['pump']:
            self.switches.did_change['dial'] = False
            if not self.switches.state('pump'):
                self._auto_brew_schedule.reset()

    def _update_pump(self):
        if self.switches.state()   == {"pump": True, "dial": _MANUAL_MODE}:
            self.solenoid.open()
            self.pump.on()
        elif self.switches.state() == {"pump": True, "dial": _HOT_MODE   }:
            self.solenoid.close()
            self.pump.on()
        elif self.switches.state() == {"pump": True, "dial": _AUTO_MODE  }:
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
        while not self.ac_power.on():
            pass
        self.leds.set(0, 1)
        self.boiler_ctrl.reset()
        self._auto_brew_schedule.reset()
        print("Machine on")

    def _update_setpoint(self):
        if self.switches.state('dial') == _AUTO_MODE or self.switches.state('dial') == _MANUAL_MODE:
            self.boiler_ctrl.update_setpoint_to(self._config["temps"]["brew"])
        elif self.switches.state('dial') == _HOT_MODE:
            self.boiler_ctrl.update_setpoint_to(self._config["temps"]["hot"])
        elif self.switches.state('dial') == _STEAM_MODE:
            self.boiler_ctrl.update_setpoint_to(self._config["temps"]["steam"])
