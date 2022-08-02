import bitstruct

import uart_bridge
import status_ids

class ACSensor(uart_bridge.UARTMessenger): 
    """
    Remote sensor object whose read() method returns True if AC power attached to 
    zerocross circut is hot.
    """
    def __init__(self, min_dwell_time : float = 0.025):
        uart_bridge.UARTMessenger.__init__(self, uart_bridge.MSG_ID_GET_AC_ON, min_dwell_time, False)
        self.response_decoder = bitstruct.compile('u8')

    def read(self)->bool:
        if uart_bridge.UARTMessenger.send(self) != status_ids.SUCCESS:
            print(f"Something went wrong with the ACStatus's UART bridge: {self.status}")
            return 0
        else:
            return (self.response_decoder.unpack(self.response)[0] != 0)

    def on(self)->bool:
        return self.read()