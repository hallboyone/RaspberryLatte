import bitstruct

import uart_bridge

class Solenoid(uart_bridge.Setter):
    def __init__(self) -> None:
        uart_bridge.Setter.__init__(self, 
            min_dwell_time = 0, 
            message_packer = bitstruct.compile('u4u4u8'),
            message_id = uart_bridge.MSG_ID_SET_SOLENOID,
            message_len = 1)
    def set(self, on_off : bool):
        self.write(on_off)
    def open(self):
        self.write(1)
    def close(self):
        self.write(0)
