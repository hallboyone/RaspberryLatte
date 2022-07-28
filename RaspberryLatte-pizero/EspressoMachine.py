from time import sleep
import configparser
import traceback
import RPi.GPIO as GPIO

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
from uart_bridge import clearUART

_STEAM_MODE  =  0
_HOT_MODE    =  1
_MANUAL_MODE =  2
_AUTO_MODE   =  3

class EspressoMachine:
    """
    Implementation of single boiler espresso machine. The basic run-loop (1) enters powered down loop if ACSensor is not on,
    (2) updates the mode, (3) ticks the boiler controller, (4) sets the LEDs,
    and (5) sets the pump value. In more detail, these 5 sections work in the following way

    (1) Enter powered down loop if turned off
        The powered down loop starts by turning off the heater, LEDs, pump, and solenoid. Then, a dwelling while loop is entered
        until ACSensor.on() returns true. Then the boiler controller and autobrew routine are reset and the power light is turned on

    (2) Update Mode
        The machine is either in steam, hot water, manual, or auto mode depending on the setting of the 1T4P dial. The dial state is
        updated and, if it changed, the boiler setpoint is adjusted accordingly and the autobrew routine is reset. Furthermore, if
        pump switch has switched to off then the autobrew routine is, again, reset.

    (3) Tick boiler controller
        Calls the tick method in the boiler's controller. This reads the latest temp and sets the boiler's PWM setting internally.

    (4) Set the LEDs
        Not yet implemented accept to turn on LED 1 when the boiler is at its setpoint.

    (5) Set the pump value
        Depending on the machine's mode, the pump (and solenoid) will take different values. When in steam mode, the pump is off
        and solenoid is closed. When in hot mode, the pump is on if the pump switch is toggled but the solenoid is closed. When in
        manual mode, the pump is on and solenoid open if the pump switch is toggled. Finally, in autobrew mode the pump's value is 
        set by the autobrew routine. As the routine progresses, the pumps value is updated based on the current leg. 
    """
    def __init__(self) -> None:
        GPIO.setmode(GPIO.BCM)
        GPIO.setup(16, GPIO.OUT)
        GPIO.output(16, 0)
        self.reset()
        sleep(1)
        clearUART()

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
        self.boiler_gains = PIDGains(0.05, 0.005, 0.25)
        self.boiler_ctrl = PID(self.boiler_gains, sensor=self.temp_sensor, output=self.heater, windup_bounds=IntegralBounds(0, 100))
        self.boiler_ctrl.update_setpoint_to(float(self._config["temps"]["brew"]))

        self._logger = Logger.Logger(sample_time=0.1)
        self._logger.add_source("temp", self.temp_sensor.read)
        self._logger.add_source("heater", lambda : self.heater.setting)
        self._logger.add_source("scale", self.scale.read)
        #self._logger.add_source("pressure", self.pressure.read)
        #self._logger.add_source("pump", lambda : self.pump.setting)
        
        self._auto_brew_routine = [
            AutoBrewScheduler.FunctionCall(self.scale.zero),
            AutoBrewScheduler.Ramp(from_pwr = 0, 
                                   to_pwr = float(self._config["autobrew"]["PRE_ON_PWR"])/100.0, 
                                   in_sec = float(self._config["autobrew"]["PRE_ON_TIME"])),
            AutoBrewScheduler.ConstantTimed(pwr = 0,  for_sec = float(self._config["autobrew"]["PRE_OFF_TIME"])),
            AutoBrewScheduler.Ramp(from_pwr = 0.0, to_pwr = 1.0, in_sec = 1),
            AutoBrewScheduler.ConstantTriggered(pwr = 1.0, trigger_callback = lambda : self.scale.read('g') >= float(self._config["autobrew"]["YIELD"]))]
        self._auto_brew_schedule = AutoBrewScheduler.AutoBrewScheduler(self._auto_brew_routine, logger = self._logger)

        self._pump_lock = False

    def run(self):
        try:
            #self.scale.zero()
            while(True):
                #self._logger.log()

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
        except Exception as e:
            print(str(e))
            print(traceback.format_exc())
            print("Shutting down")
            self.leds.set_all(0,0,0)
            self.heater.off()
            self.solenoid.close()
            self.pump.off()

    def _update_mode(self):
        (dial_changed, pump_changed) = self.switches.update()
        if dial_changed:
            self._update_setpoint()
            self._pump_lock = True
            if (self.switches.state('dial') == _AUTO_MODE):
                self._auto_brew_schedule.reset()
        
        if not self.switches.state('pump'):
            self._pump_lock = False
            if pump_changed:
                self._auto_brew_schedule.reset()

    def _update_pump(self):
        if self._pump_lock:
            self.solenoid.close()
            self.pump.off()
        elif self.switches.state()   == {"pump": True, "dial": _MANUAL_MODE}:
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
            self.boiler_ctrl.update_setpoint_to(float(self._config["temps"]["brew"]))
        elif self.switches.state('dial') == _HOT_MODE:
            self.boiler_ctrl.update_setpoint_to(float(self._config["temps"]["hot"]))
        elif self.switches.state('dial') == _STEAM_MODE:
            self.boiler_ctrl.update_setpoint_to(float(self._config["temps"]["steam"]))

    def reset(self):
        GPIO.output(16, 1)
        sleep(0.01)
        GPIO.output(16, 0)
