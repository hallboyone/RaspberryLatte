import bitstruct

import uart_bridge
import status_ids

class ACSensor(uart_bridge.Getter): 
    """
    Remote sensor object whose read() method returns True if AC power attached to 
    zerocross circut is hot.
    """
    def __init__(self, min_dwell_time : float = 0.025):
        uart_bridge.Getter.__init__(self, min_dwell_time, False,
                                    request_message = bitstruct.pack('u4u4', uart_bridge.MSG_ID_GET_AC_ON, 0),
                                    response_decoder = bitstruct.compile('u8'))

    def read(self)->bool:
        if uart_bridge.Getter.read(self) != status_ids.SUCCESS:
            print(f"Something went wrong with the ACStatus's UART bridge: {self.status}")
            return 0
        else:
            return (self.unpacked_response[0] != 0)

    def on(self)->bool:
        return self.read()