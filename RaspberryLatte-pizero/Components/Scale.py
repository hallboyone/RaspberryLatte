import bitstruct

from PID import PIDSensor
import uart_bridge

class Scale(uart_bridge.Getter, PIDSensor): 
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
    _WEIGHT_REQUEST = bitstruct.pack('u4u4', uart_bridge.MSG_ID_GET_WEIGHT, 0)
    _WEIGHT_DECODER = bitstruct.compile('u24')

    def __init__(self, min_dwell_time : float = 0.1) -> None:
        super().__init__(min_dwell_time, self._WEIGHT_REQUEST, self._WEIGHT_DECODER)
        self._origin = 0

    def zero(self):
        uart_bridge.Getter.read(self)
        self._origin = self._last_reading.body[0]

    def read(self, unit = 'g') -> float:
        uart_bridge.Getter.read(self)
        val_g = -0.000152968191*(self._last_reading.val[0] - self._origin)
        if unit == 'g':
            return val_g
        elif unit == 'oz':
            return 0.0352739619 * val_g