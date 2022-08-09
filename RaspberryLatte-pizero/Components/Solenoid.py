import bitstruct

import uart_bridge
import status_ids
class Solenoid(uart_bridge.UARTMessenger):
    def __init__(self) -> None:
        uart_bridge.UARTMessenger.__init__(self, uart_bridge.MSG_ID_SET_SOLENOID, min_dwell_time = 0, avoid_repeat_sends = True)
        self._setting_encoding = bitstruct.compile('u8u8')
        self.set(0)

    def set(self, on_off : bool):
        msg_body = self._setting_encoding.pack(0, on_off)
        if uart_bridge.UARTMessenger.send(self, msg_body, False) != status_ids.SUCCESS:
            print(f"Something went wrong setting the solenoid!: {self.status}")
        
    def open(self):
        self.set(1)
    def close(self):
        self.set(0)
