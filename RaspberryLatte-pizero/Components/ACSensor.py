from asyncio import constants
import bitstruct

import uart_bridge

class ACSensor(uart_bridge.Getter): 
    """
    Remote sensor object whose read() method returns True if AC power attached to 
    zerocross circut is hot.
    """
    _AC_REQUEST = bitstruct.pack('u4u4', uart_bridge.MSG_ID_GET_AC_ON, 0)
    _AC_DECODER = bitstruct.compile('u8')

    def __init__(self, min_dwell_time : float = 0.1) -> None:
        super().__init__(min_dwell_time, self._AC_REQUEST, self._AC_DECODER)

    def read(self)->bool:
        uart_bridge.Getter.read(self)
        return bool(self._last_reading.body[0])

    def on(self)->bool:
        return self.read()