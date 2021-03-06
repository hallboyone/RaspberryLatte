import bitstruct

from PID import PIDSensor
import uart_bridge
import status_ids

class Scale(uart_bridge.UARTMessenger, PIDSensor): 
    """
    Retreives the scale state from the espresso machine interfacing with the pico 
    firmware.

    Scale.zero()\\
    Records the current scale value internally. This value is subtracted during future
    calls to self.read().

    Scale.read(unit : string = 'g')\\
    Updates the scale's value (if min-DT expired) and returns the current weight relative
    to the last zeroed amount. The weight is returned in the units requested (either 'g' or
    'oz')
    """

    def __init__(self, min_dwell_time : float = 0.1) -> None:
        uart_bridge.UARTMessenger.__init__(self, 0.1, False)
        self.request_msg = bitstruct.pack('u4u4', uart_bridge.MSG_ID_GET_WEIGHT, 0)
        self._decoder = bitstruct.compile('u24')
        self._origin = 0

    def zero(self):
        uart_bridge.UARTMessenger.send(self, self.request_msg)
        self._origin = self._decoder.unpack(self.response)[0]

    def read(self, unit = 'g') -> float:
        uart_bridge.UARTMessenger.send(self, self.request_msg)
        if self.status != status_ids.SUCCESS:
            print(f"Something went wrong with the scale's UART bridge: {self.status}")
            return 0
        else:
            val_g = 0.000152710615479*(self._decoder.unpack(self.response)[0] - self._origin)
            if unit == 'g':
                return val_g
            else:
                return 0.0352739619 * val_g
