import uart_bridge
import PID
import time

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
        
class LMT01(PID.PIDSensor):
    def __init__(self) -> None:
        super().__init__()
    
    def read(self) -> float:
        return uart_bridge.get_tempurature().in_C()