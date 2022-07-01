import bitstruct

from PID import PIDSensor
import uart_bridge
import status_ids

class TempSensor(uart_bridge.Getter, PIDSensor): 
    def __init__(self) -> None:
        uart_bridge.Getter.__init__(self, 0.1, False, 
                                    request_message = bitstruct.pack('u4u4', uart_bridge.MSG_ID_GET_TEMP, 0),
                                    response_decoder = bitstruct.compile('u16'))

    def read(self, unit = 'C') -> float:
        if uart_bridge.Getter.read(self) != status_ids.SUCCESS:
            print(f"Something went wrong with the thermostat's UART bridge: {self.status}")
            return 0
        elif unit == 'C':
            return 0.0625 * self.unpacked_response[0]
        else:
            return 0.1125* self.unpacked_response[0]