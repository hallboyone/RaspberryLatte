import bitstruct

import uart_bridge
import status_ids

class LEDs(uart_bridge.UARTMessenger):
    """
    Control the 3 LEDs attached to the pico. They can all be set using set_all or set individually
    using the set method which takes the LED index and its value. 
    """
    _BINARY_OUT_GROUP = 0

    def __init__(self) -> None:
        uart_bridge.UARTMessenger.__init__(self, uart_bridge.MSG_ID_SET_LEDS, 0, True)
        self._msg_body_encoding = bitstruct.compile('u8u8u8')
        self.set_all([0,0,0])

    def set_all(self, led_vals : list[bool]):
        for i in range(3):
            self.set(i, led_vals[i])

    def set(self, led_idx : int, state : bool):
        """Set the LED indicated by led_idx to the provided state"""
        if led_idx > 2 or led_idx < 0:
            raise RuntimeError("LED index must be in {0,1,2}")
        msg_body = self._msg_body_encoding.pack(self._BINARY_OUT_GROUP, led_idx, state)
        if uart_bridge.UARTMessenger.send(self, msg_body) != status_ids.SUCCESS:
            print(f"Something went wrong with the LED's UART bridge: {self.status}")