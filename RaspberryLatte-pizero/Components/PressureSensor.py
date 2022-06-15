import bitstruct

from PID import PIDSensor
import uart_bridge
import status_ids
class PressureSensor(uart_bridge.UARTMessenger, PIDSensor): 
    
    def __init__(self) -> None:
        uart_bridge.UARTMessenger.__init__(self, 0.1, False)
        self.request_msg = bitstruct.pack('u4u4', uart_bridge.MSG_ID_GET_PRESSURE, 0)
        self._decoder = bitstruct.compile('u16')

    def read(self, unit = 'bar') -> float:
        uart_bridge.UARTMessenger.send(self, self.request_msg)
        if self.status != status_ids.SUCCESS:
            print(f"Something went wrong with the pressure sensor's UART bridge: {self.status}")
            return 0
        if unit == 'bar':
            return 0.001 * self._decoder.unpack(self.response)[0]
        else:
            return self._decoder.unpack(self.response)[0]