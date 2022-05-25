import bitstruct

import uart_bridge

class Switches(uart_bridge.Getter): 
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
    
    _SWITCH_REQUEST = bitstruct.pack('u4u4', uart_bridge.MSG_ID_GET_SWITCH, 0)
    _SWITCH_DECODER = bitstruct.compile('u8u8')

    def __init__(self, min_dwell_time : float = 0.1) -> None:
        super().__init__(min_dwell_time, self._SWITCH_REQUEST, self._SWITCH_DECODER)
        self.read()
        self._prev_readings = self._last_reading
        self.did_change = {'pump':False, 'dial':False}

    def read(self, switch = 'all'):
        uart_bridge.Getter.read(self)
        self.did_change['pump'] = self.did_change['pump'] or (self._prev_readings.body[0]==self._last_reading.body[0])
        self.did_change['dial'] = self.did_change['dial'] or (self._prev_readings.body[1]==self._last_reading.body[1])
        return self.state(switch)

    def state(self, switch = 'all'):
        if switch == 'pump':
            return self._last_reading.body[0]
        elif switch == 'dial':
            return self._last_reading.body[1]
        else:
            return {'pump':self._last_reading.body[0], 'dial':self._last_reading.body[1]}