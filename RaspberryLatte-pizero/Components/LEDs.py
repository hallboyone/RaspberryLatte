import bitstruct

import uart_bridge

class LEDs(uart_bridge.Setter):
    """
    Control the 3 LEDs attached to the pico. They can all be set using set_all or set individually
    using the set method which takes the LED index and its value. 
    """
    def __init__(self) -> None:
        uart_bridge.Setter.__init__(self, 
            min_dwell_time = 0, 
            message_packer = bitstruct.compile('u4u4u8'),
            message_id = uart_bridge.MSG_ID_SET_LEDS,
            message_len = 1)

    def set_all(self, led0_val: bool, led1_val: bool, led2_val: bool):
        self.write(led2_val<<2 | led1_val<<1 | led0_val<<0, force = False)

    def set(self, led_idx : int, state : bool):
        """Set the LED indicated by led_idx to the provided state"""
        if led_idx > 2 or led_idx < 0:
            raise RuntimeError("LED index must be in {0,1,2}")
        self.write(self._last_setting.val & ~(1<<led_idx) | state<<led_idx, force = False)