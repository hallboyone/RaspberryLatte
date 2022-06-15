import bitstruct

import uart_bridge
import status_ids

from PID import PIDOutput
from util import Bounds

# class Heater(uart_bridge.Setter, PIDOutput):
#     """ 
#     Object representing the boiler heater. Calling the write(self, val : float) method
#     will send a message to the pico over the uart bridge that will set the heater's PWM
#     duty cycle. Writing 0 is full off while 1 is full on. 
#     """
#     _pwr_bounds = Bounds(0, 1)

#     def __init__(self) -> None:
#         uart_bridge.Setter.__init__(self, 
#             min_dwell_time = 0.05, 
#             message_packer = bitstruct.compile('u4u4u8'),
#             message_id = uart_bridge.MSG_ID_SET_HEATER,
#             message_len = 1)

#     def write(self, val : float):
#         uart_bridge.Setter.write(self, 63*self._pwr_bounds.clip(val), force = False)

#     def off(self) -> None:
#         uart_bridge.Setter.write(self, 0, force = True)

class Heater(uart_bridge.UARTMessenger, PIDOutput):
    """ 
    Object representing the boiler heater. Calling the write(self, val : float) method
    will send a message to the pico over the uart bridge that will set the heater's PWM
    duty cycle. Writing 0 is full off while 1 is full on. 
    """
    _pwr_bounds = Bounds(0, 1)

    def __init__(self) -> None:
        uart_bridge.UARTMessenger.__init__(self, min_dwell_time = 0.05, avoid_repeat_sends = True)
        self.msg_buf = bytearray(2)
        bitstruct.pack_into('u4u4', self.msg_buf, 0, uart_bridge.MSG_ID_SET_HEATER, 1)
        self.write(0)

    def write(self, val : float, force = False):
        self.setting = round(63*self._pwr_bounds.clip(val))
        bitstruct.pack_into('u8', self.msg_buf, 8, self.setting)
        uart_bridge.UARTMessenger.send(self, self.msg_buf, force)
        if self.status != status_ids.SUCCESS:
            print(f"Something went wrong setting the heater!: {self.status}")
        return self.setting

    def off(self) -> None:
        self.write(0, True)