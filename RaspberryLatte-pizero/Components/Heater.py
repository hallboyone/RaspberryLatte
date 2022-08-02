import bitstruct

import uart_bridge
import status_ids

from PID import PIDOutput
from util import Bounds

class Heater(uart_bridge.UARTMessenger, PIDOutput):
    """ 
    Object representing the boiler heater. Calling the write(self, val : float) method
    will send a message to the pico over the uart bridge that will set the heater's PWM
    duty cycle. Writing 0 is full off while 1 is full on. 
    """
    _pwr_bounds = Bounds(0, 1)

    def __init__(self) -> None:
        uart_bridge.UARTMessenger.__init__(self, uart_bridge.MSG_ID_SET_HEATER, min_dwell_time = 0.05, avoid_repeat_sends = True)
        self._setting_encoding = bitstruct.compile('u8')
        
        self.write(0)

    def write(self, val : float, force = False):
        msg_body = self._setting_encoding.pack(round(63*self._pwr_bounds.clip(val)))
        if uart_bridge.UARTMessenger.send(self, msg_body, force) != status_ids.SUCCESS:
            print(f"Something went wrong setting the heater!: {self.status}")
            return 0
        self.setting = self._setting_encoding.unpack(self.response)[0]
        return self.setting

    def off(self) -> None:
        self.write(0, True)