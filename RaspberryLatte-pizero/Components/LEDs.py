import bitstruct

import uart_bridge
import status_ids

class LEDs(uart_bridge.UARTMessenger):
    """
    Control the 3 LEDs attached to the pico. They can all be set using set_all or set individually
    using the set method which takes the LED index and its value. 
    """
    def __init__(self) -> None:
        uart_bridge.UARTMessenger.__init__(self, min_dwell_time = 0)
        self.msg_buf = bytearray(2)
        bitstruct.pack_into('u4u4', self.msg_buf, 0, uart_bridge.MSG_ID_SET_LEDS, 1)
        self.set_all(0,0,0)

    def send(self):
        bitstruct.pack_into('u8', self.msg_buf, 8, self.last_setting)
        uart_bridge.UARTMessenger.send(self, self.msg_buf)
        if self.status != status_ids.SUCCESS:
            print(f"Something went wrong with the LED's UART bridge: {self.status}")

    def set_all(self, led0_val: bool, led1_val: bool, led2_val: bool):
        self.last_setting = led2_val<<2 | led1_val<<1 | led0_val<<0
        self.send()

    def set(self, led_idx : int, state : bool):
        """Set the LED indicated by led_idx to the provided state"""
        if led_idx > 2 or led_idx < 0:
            raise RuntimeError("LED index must be in {0,1,2}")
        self.last_setting = self.last_setting & ~(1<<led_idx) | state<<led_idx
        self.send()