import bitstruct

from Getter import Getter
from PID import PIDSensor
import uart_bridge

class Scale(Getter, PIDSensor): 
    _WEIGHT_REQUEST = bitstruct.pack('u4u4', uart_bridge._MSG_ID_GET_WEIGHT, 0)
    _WEIGHT_DECODER = bitstruct.compile('u24')

    def __init__(self, min_dwell_time : float = 0.1) -> None:
        super().__init__(min_dwell_time, self._WEIGHT_REQUEST, self._WEIGHT_DECODER)
        self._origin = 0

    def zero(self):
        Getter.read(self)
        self._origin = self._last_reading.body[0]

    def read(self, unit = 'g') -> float:
        Getter.read(self)
        val_g = -0.000152968191*(self._last_reading.body[0] - self._origin)
        if unit == 'g':
            return val_g
        elif unit == 'oz':
            return 0.0352739619 * val_g