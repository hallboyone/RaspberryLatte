import bitstruct

import uart_bridge
import status_ids

class Switches(uart_bridge.UARTMessenger): 
    """
    Retreives the switch states from the espresso machine interfacing with the pico 
    firmware. Handles one switch and one dial. 
    
    Switches.state(switch : string = 'all')\\
    Return the last read switch state. If 'all' is requested, a dictionary indexed by
    'pump' and 'dial' are returned with values indicating the current value (1-4
    in the case of the dial, 0-1 for the pump)

    Switches.read(switch : string = 'all')\\
    Update the switch states if the min-DT has expired (default to 0.1s). Updates internal
    flag, did_change indicating of values changed. Flag remain set until cleared by
    calling script. Returns the results of self.state(switch) after update attempt.
    """

    def __init__(self) -> None:
        uart_bridge.UARTMessenger.__init__(self, uart_bridge.MSG_ID_GET_SWITCH, 0.1, False)
        self._switch_decoder = bitstruct.compile('u8u8')
        self.did_change = {'pump':False, 'dial':False}
        self.reading = None

    def update(self):
        if uart_bridge.UARTMessenger.send(self) != status_ids.SUCCESS:
            print(f"Something went wrong with the switches' UART bridge: {self.status}")
            return False
        new_reading = self._switch_decoder.unpack(self.response)
        switch_changed = (self.reading is None) or (new_reading[0]!=self.reading[0])
        dial_changed = (self.reading is None) or (new_reading[1]!=self.reading[1])
        self.reading = new_reading
        return (dial_changed, switch_changed)

    def state(self, switch = 'all'):
        if switch == 'pump':
            return self.reading[0]
        elif switch == 'dial':
            return self.reading[1]
        else:
            return {'pump':self.reading[0], 'dial':self.reading[1]}