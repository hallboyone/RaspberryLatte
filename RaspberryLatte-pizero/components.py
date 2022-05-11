import uart_bridge
from PID import PIDSensor, PIDOutput
import time
from util import Bounds

class GetterComponent:
    """
    Abstract class to handles retrieving values over the uart_bridge. If the value was recently retrieved (not more than
    min_dwell_time seconds ago), then the value is just reused.
    """
    _last_reading : uart_bridge.Reading = None
    _getter = None

    def __init__(self, min_dwell_time : float, getter) -> None:
        super().__init__()
        self._min_dwell_time = min_dwell_time
        self._getter = getter
    
    def update(self):
        if (self._last_reading) == None or (time.time() - self._last_reading.timestamp > self._min_dwell_time):
            self._last_reading = self._getter()

class LMT01(GetterComponent, PIDSensor):
    _last_reading : uart_bridge.TempuratureReading

    def __init__(self, min_dwell_time : float = 0.1) -> None:
        super().__init__(min_dwell_time, uart_bridge.get_tempurature)
    
    def read(self) -> float:
        super().update()
        return self._last_reading.in_C()

class PressureSensor(GetterComponent, PIDSensor):
    _last_reading : uart_bridge.PressureReading
    def __init__(self, min_dwell_time : float = 0.1) -> None:
        super().__init__(min_dwell_time, uart_bridge.get_pressure)
    
    def read(self) -> float:
        super().update()
        return self._last_reading.in_bar()

class Scale(GetterComponent, PIDSensor):    
    _last_reading : uart_bridge.ScaleReading = None
    def __init__(self, min_dwell_time : float = 0.1) -> None:
        super().__init__(min_dwell_time, uart_bridge.get_weight)
    
    def read(self) -> float:
        super().update()
        return self._last_reading.in_g()

class Switches(GetterComponent):    
    _last_reading : uart_bridge.SwitchReading = None
    def __init__(self, min_dwell_time : float = 0.1) -> None:
        super().__init__(min_dwell_time, uart_bridge.get_switches)
    
    def read(self, switch = None):
        super().update()
        if switch == "pump":
            return self._last_reading.pump()
        elif switch == "dial" or switch == "mode":
            return self._last_reading.dial()
        else:
            return {"pump" : self._last_reading.pump(), "dial" : self._last_reading.dial()}

class ACStatus(GetterComponent):    
    _last_reading : uart_bridge.ACReading = None
    def __init__(self, min_dwell_time : float = 0.1) -> None:
        super().__init__(min_dwell_time, uart_bridge.get_ac_on)
    
    def read(self) -> bool:
        super().update()
        return self._last_reading.val


class Heater(PIDOutput):
    _pwr_bounds = Bounds(0, 63)
    def __init__(self) -> None:
        super().__init__()
        self._prev_value = None
    
    def write(self, val: float) -> float:
        val = self._pwr_bounds.clip(round(val))
        
        if self._prev_value == None or self._prev_value != val:
            uart_bridge.set_heater_to(val)
            self._prev_value = val
        print(f"Setting heater to {round(val/0.63,2)}% power")
        return val
        
    def off(self) -> None:
        self._prev_value = 0
        uart_bridge.set_heater_to(0)

class LEDs:    
    def set_all(self, led0_val, led1_val, led2_val):
        uart_bridge.set_leds([0,1,2], [led0_val, led1_val, led2_val])

    def set(self, led_num : int, state : bool):
        uart_bridge.set_leds(led_num, state)

class Pump(PIDOutput):
    _pwr_bounds = Bounds(60, 127, allow_zero = True)
    def set(self, pwr : float):
        uart_bridge.set_pump_to(self._pwr_bounds.clip(round(pwr)))
    def on(self, pwr : float = 127):
        self.set(pwr)
    def off(self):
        self.set(0)

class Solenoid:
    def set(self, on_off : bool):
        uart_bridge.set_solenoid_to(on_off)
    def open(self):
        self.set(True)
    def close(self):
        self.set(False)
