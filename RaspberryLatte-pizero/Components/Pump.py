import bitstruct

import uart_bridge
import status_ids
from PID import PIDOutput
from util import Bounds

class Pump(uart_bridge.UARTMessenger, PIDOutput):
    _pwr_bounds = Bounds(0, 1, allow_zero = True)

    def __init__(self) -> None:
        uart_bridge.UARTMessenger.__init__(self, uart_bridge.MSG_ID_SET_PUMP, min_dwell_time = 0.005, avoid_repeat_sends = True)
        self._setting_encoding = bitstruct.compile('u8')

        self.write(0)

    def write(self, val : float, force : bool = False):
        if self._pwr_bounds.clip(val) == 0:
            msg_body = self._setting_encoding.pack(0)
        else:
            msg_body = self._setting_encoding.pack(round(self._pwr_bounds.clip(val)*67 + 60))
            
        if uart_bridge.UARTMessenger.send(self, msg_body, force) != status_ids.SUCCESS:
            print(f"Something went wrong setting the pump!: {self.status}")
            return 0
        self.setting = self._setting_encoding.unpack(self.response)[0]
        return self.setting
    
    def set(self, pwr : float):
        self.write(pwr)
    
    def on(self, pwr : float = 1.0):
        self.write(pwr, force = True)

    def off(self):
        self.write(0, force = True)