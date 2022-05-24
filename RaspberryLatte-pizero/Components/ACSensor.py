import bitstruct

from Getter import Getter
import uart_bridge

class ACSensor(Getter): 
    _AC_REQUEST = bitstruct.pack('u4u4', uart_bridge._MSG_ID_GET_AC_ON, 0)
    _AC_DECODER = bitstruct.compile('u8')

    def __init__(self, min_dwell_time : float = 0.1) -> None:
        super().__init__(min_dwell_time, self._AC_REQUEST, self._AC_DECODER)

    def read(self)->bool:
        Getter.read(self)
        return bool(self._last_reading.body[0])