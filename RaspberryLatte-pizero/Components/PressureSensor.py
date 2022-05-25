import bitstruct

from PID import PIDSensor
import uart_bridge

class PressureSensor(uart_bridge.Getter, PIDSensor): 
    _PRESSURE_REQUEST = bitstruct.pack('u4u4', uart_bridge.MSG_ID_GET_PRESSURE, 0)
    _PRESSURE_DECODER = bitstruct.compile('u16')
    
    def __init__(self, min_dwell_time : float = 0.1) -> None:
        super().__init__(min_dwell_time, self._PRESSURE_REQUEST, self._PRESSURE_DECODER)

    def read(self, unit = 'bar') -> float:
        uart_bridge.Getter.read(self)
        if unit == 'bar':
            return 0.001 * self._last_reading.body[0]
        else:
            return self._last_reading.body[0]