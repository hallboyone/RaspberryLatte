import bitstruct

from PID import PIDSensor
import uart_bridge
import status_ids

class TempSensor(uart_bridge.UARTMessenger, PIDSensor): 
    _TEMP_REQUEST = bitstruct.pack('u4u4', uart_bridge.MSG_ID_GET_TEMP, 0)
    _TEMP_DECODER = bitstruct.compile('u16')

    def __init__(self) -> None:
        uart_bridge.UARTMessenger.__init__(self, 0.1, False)
        self.request_msg = bitstruct.pack('u4u4', uart_bridge.MSG_ID_GET_TEMP, 0)
        self._decoder = bitstruct.compile('u16')

    def read(self, unit = 'C') -> float:
        uart_bridge.UARTMessenger.send(self, self.request_msg)
        if self.status != status_ids.SUCCESS:
            print(f"Something went wrong with the thermostat's UART bridge: {self.status}")
            return 0
        elif unit == 'C':
            return 0.0625 * self._decoder.unpack(self.response)[0]
        else:
            return 0.1125 * self._decoder.unpack(self.response)[0] + 32