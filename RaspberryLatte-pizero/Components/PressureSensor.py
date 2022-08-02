import bitstruct

from PID import PIDSensor
import uart_bridge
import status_ids
class PressureSensor(uart_bridge.UARTMessenger, PIDSensor): 
    
    def __init__(self) -> None:
        uart_bridge.UARTMessenger.__init__(self, uart_bridge.MSG_ID_GET_PRESSURE, 0.05, False)
        self.pressure_decoder = bitstruct.compile('u16')

    def read(self, unit = 'bar') -> float:
        if uart_bridge.UARTMessenger.send(self) != status_ids.SUCCESS:
            print(f"Something went wrong with the pressure sensor's UART bridge: {self.status}")
            return 0
        if unit == 'bar':
            return 0.001 * self.pressure_decoder.unpack(self.response)[0]
        else:
            return self.pressure_decoder.unpack(self.response)[0]