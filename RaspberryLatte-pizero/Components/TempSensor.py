import bitstruct

from PID import PIDSensor
import uart_bridge
import status_ids

class TempSensor(uart_bridge.UARTMessenger, PIDSensor): 
    def __init__(self) -> None:
        uart_bridge.UARTMessenger.__init__(self, uart_bridge.MSG_ID_GET_TEMP, 0.1, False)
        self._temp_decoder = bitstruct.compile('u16')

    def read(self, unit = 'C') -> float:
        if uart_bridge.UARTMessenger.send(self) != status_ids.SUCCESS:
            print(f"Something went wrong with the thermostat's UART bridge: {self.status}")
            return 0
        elif unit == 'C':
            return 0.0625 * self._temp_decoder.unpack(self.response)[0]
        else:
            return 0.1125 * self._temp_decoder.unpack(self.response)[0]