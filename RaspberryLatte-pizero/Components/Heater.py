import bitstruct

import uart_bridge
from PID import PIDOutput
from util import Bounds

class Heater(uart_bridge.Setter, PIDOutput):
    """ 
    Object representing the boiler heater. Calling the write(self, val : float) method
    will send a message to the pico over the uart bridge that will set the heater's PWM
    duty cycle. Writing 0 is full off while 1 is full on. 
    """
    _pwr_bounds = Bounds(0, 1)

    def __init__(self) -> None:
        uart_bridge.Setter.__init__(self, 
            min_dwell_time = 0.05, 
            message_packer = bitstruct.compile('u4u4u8'),
            message_id = uart_bridge.MSG_ID_SET_HEATER,
            message_len = 1)

    def write(self, val : float):
        uart_bridge.Setter.write(self, 63*self._pwr_bounds.clip(val), force = False)

    def off(self) -> None:
        uart_bridge.Setter.write(self, 0, force = True)