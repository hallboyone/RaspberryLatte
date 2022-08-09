import bitstruct

import uart_bridge
import status_ids

class PumpSwitch(uart_bridge.UARTMessenger): 
    def __init__(self) -> None:
        uart_bridge.UARTMessenger.__init__(self, uart_bridge.MSG_ID_GET_SWITCH, 0.1, False)
        self._switch_decoder = bitstruct.compile('u8')
        self.changed = False
        self.state = None

    def read(self) -> bool:
        if uart_bridge.UARTMessenger.send(self) != status_ids.SUCCESS:
            print(f"Something went wrong with the switches' UART bridge: {self.status}")
            return False
        new_reading = (self._switch_decoder.unpack(self.response)[0] != 0)
        self.changed = (self.state is None) or (new_reading != self.state)
        self.state = new_reading
        return self.state