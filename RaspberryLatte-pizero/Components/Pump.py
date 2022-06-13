import bitstruct

import uart_bridge
from PID import PIDOutput
from util import Bounds

class Pump(uart_bridge.Setter, PIDOutput):
    _pwr_bounds = Bounds(60, 127, allow_zero = True)

    def __init__(self) -> None:
        uart_bridge.Setter.__init__(self, 
            min_dwell_time = 0.005, 
            message_packer = bitstruct.compile('u4u4u8'),
            message_id = uart_bridge.MSG_ID_SET_PUMP,
            message_len = 1)

    def write(self, val : float, force : bool = False):
        uart_bridge.Setter.write(self, self._pwr_bounds.clip(val), force)
    
    def set(self, pwr : float):
        self.write(pwr)
    
    def on(self, pwr : float = 127):
        self.write(pwr, force = True)

    def off(self):
        self.write(0, force = True)