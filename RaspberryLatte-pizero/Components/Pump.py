import bitstruct

import uart_bridge
import status_ids
from PID import PIDOutput
from util import Bounds

class Pump(uart_bridge.UARTMessenger, PIDOutput):
    _pwr_bounds = Bounds(60, 127, allow_zero = True)

    def __init__(self) -> None:
        uart_bridge.UARTMessenger.__init__(self, min_dwell_time = 0.005, avoid_repeat_sends = True)
        self.msg_buf = bytearray(2)
        bitstruct.pack_into('u4u4', self.msg_buf, 0, uart_bridge.MSG_ID_SET_PUMP, 1)
        self.write(0)

    def write(self, val : float, force : bool = False):
        if val==0:
            self.setting = 0
        else:
            self.setting = round(self._pwr_bounds.clip(val*67 + 60))
            
        bitstruct.pack_into('u8', self.msg_buf, 8, self.setting)
        uart_bridge.UARTMessenger.send(self, self.msg_buf, force)
        if self.status != status_ids.SUCCESS:
            print(f"Something went wrong setting the pump!: {self.status}")
        return self.setting
    
    def set(self, pwr : float):
        self.write(pwr)
    
    def on(self, pwr : float = 1.0):
        self.write(pwr, force = True)

    def off(self):
        self.write(0, force = True)