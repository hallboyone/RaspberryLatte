import bitstruct

from Getter import Getter
import uart_bridge

class Switches(Getter): 
    _SWITCH_REQUEST = bitstruct.pack('u4u4', uart_bridge._MSG_ID_GET_SWITCH, 0)
    _SWITCH_DECODER = bitstruct.compile('u8u8')

    def __init__(self, min_dwell_time : float = 0.1) -> None:
        super().__init__(min_dwell_time, self._SWITCH_REQUEST, self._SWITCH_DECODER)
        self.read()
        self._prev_readings = self._last_reading
        self.did_change = {'pump':False, 'dial':False}

    def read(self, switch = 'all'):
        Getter.read(self)
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