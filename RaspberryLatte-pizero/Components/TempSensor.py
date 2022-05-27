import bitstruct

from PID import PIDSensor
import uart_bridge

class TempSensor(uart_bridge.Getter, PIDSensor): 
    _TEMP_REQUEST = bitstruct.pack('u4u4', uart_bridge.MSG_ID_GET_TEMP, 0)
    _TEMP_DECODER = bitstruct.compile('u16')

    def __init__(self, min_dwell_time : float = 0.1) -> None:
        super().__init__(min_dwell_time, self._TEMP_REQUEST, self._TEMP_DECODER)

    def read(self, unit = 'C') -> float:
        uart_bridge.Getter.read(self)
        if unit == 'C':
            return 0.0625 * self._last_reading.val[0]
        else:
            return 0.1125 * self._last_reading.val[0] + 32