import bitstruct

import uart_bridge
from PID import PIDOutput
from util import Bounds

class Heater(uart_bridge.Setter, PIDOutput):
    _pwr_bounds = Bounds(0, 63)

    def __init__(self) -> None:
        uart_bridge.Setter.__init__(self, 
            min_dwell_time = 0.05, 
            message_packer = bitstruct.compile('u4u4u8'),
            message_id = uart_bridge.MSG_ID_SET_HEATER,
            message_len = 1)

    def write(self, val : float):
        uart_bridge.Setter.write(val, force = False)

    def off(self) -> None:
        self.write(0, force = True)