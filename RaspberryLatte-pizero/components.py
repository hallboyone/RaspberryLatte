import uart_bridge
import PID
import time

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
        if (self._last_reading) == None or (self._last_reading.timestamp - time.time() > self._min_dwell_time):
            self._last_reading = self._getter()

class Heater(PID.PIDOutput):
    def __init__(self) -> None:
        super().__init__()
        self._prev_value = None
        self._input_bounds = {"lower":0, "upper":63}
    
    def write(self, val: float) -> float:
        val = round(val)
        if val < self._input_bounds["lower"]:
            val = self._input_bounds["lower"]
        elif val > self._input_bounds["upper"]:
            val = self._input_bounds["upper"]
        
        if self._prev_value == None or self._prev_value != val:
            uart_bridge.set_heater_to(val)
            self._prev_value = val
        print(f"Setting heater to {round(val/0.63,2)}% power")
        return val
        
    def off(self) -> None:
        self._prev_value = 0
        uart_bridge.set_heater_to(0)

class LEDs:
    def __init__(self, pins : [int]):
        self._pins = pins
    
    def set(self, state : [bool]):
        uart_bridge.set_gpio_to([self._pins], state)

    
class LMT01(GetterComponent, PID.PIDSensor):
    _last_reading : uart_bridge.TempuratureReading

    def __init__(self, min_dwell_time : float = 0.1) -> None:
        super().__init__(min_dwell_time, uart_bridge.get_tempurature)
    
    def read(self) -> float:
        super().update()
        return self._last_reading.in_C()

class PressureSensor(GetterComponent, PID.PIDSensor):
    _last_reading : uart_bridge.PressureReading
    def __init__(self, min_dwell_time : float = 0.1) -> None:
        super().__init__(min_dwell_time, uart_bridge.get_pressure)
    
    def read(self) -> float:
        super().update()
        return self._last_reading.in_bar()

class Scale(GetterComponent, PID.PIDSensor):    
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
    
    def read(self) -> dict[str,int]:
        super().update()
        return {"pump" : self._last_reading.pump(), "dial" : self._last_reading.dial()}

class ACStatus(GetterComponent):    
    _last_reading : bool = None
    def __init__(self, min_dwell_time : float = 0.1) -> None:
        super().__init__(min_dwell_time, uart_bridge.get_ac_on)
    
    def read(self) -> bool:
        super().update()
        return self._last_reading