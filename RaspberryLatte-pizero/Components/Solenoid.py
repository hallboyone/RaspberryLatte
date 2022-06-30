import bitstruct

import uart_bridge
import status_ids
class Solenoid(uart_bridge.UARTMessenger):
    def __init__(self) -> None:
        uart_bridge.UARTMessenger.__init__(self, min_dwell_time = 0, avoid_repeat_sends = True)
        self.msg_buf = bytearray(2)
        bitstruct.pack_into('u4u4', self.msg_buf, 0, uart_bridge.MSG_ID_SET_SOLENOID, 1)
        self.set(0)

    def set(self, on_off : bool):
        bitstruct.pack_into('u8', self.msg_buf, 8, on_off)
        uart_bridge.UARTMessenger.send(self, self.msg_buf, False)
        if self.status != status_ids.SUCCESS:
            print(f"Something went wrong setting the solenoid!: {self.status}")
        
    def open(self):
        self.set(1)
    def close(self):
        self.set(0)
