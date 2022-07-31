import bitstruct

import uart_bridge
import status_ids

class LEDs(uart_bridge.UARTMessenger):
    """
    Control the 3 LEDs attached to the pico. They can all be set using set_all or set individually
    using the set method which takes the LED index and its value. 
    """
    def __init__(self) -> None:
        uart_bridge.UARTMessenger.__init__(self, 0, True)
        self.msg_buf = bytearray(4)
        bitstruct.pack_into('u4u4u8', self.msg_buf, 0, uart_bridge.MSG_ID_SET_LEDS, 3, 0)
        self.set_all([0,0,0])

    def set_all(self, led_vals : list[bool]):
        for i in range(3):
            self.set(i, led_vals[i])

    def set(self, led_idx : int, state : bool):
        """Set the LED indicated by led_idx to the provided state"""
        if led_idx > 2 or led_idx < 0:
            raise RuntimeError("LED index must be in {0,1,2}")
        bitstruct.pack_into('u8u8', self.msg_buf, 16, led_idx, state)
        uart_bridge.UARTMessenger.send(self, self.msg_buf)
        if self.status != status_ids.SUCCESS:
            print(f"Something went wrong with the LED's UART bridge: {self.status}")