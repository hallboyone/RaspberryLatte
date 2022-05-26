"""
Provides getter and setter classes for retreiving sensor data and sending commands over uart
to pico running RaspberryLatte firmware. The module should be primarly used by inheriting
the Getter and Setter classes which internally call the _send_over_uart function.
"""
import serial
import bitstruct
import time

MSG_ID_SET_LEDS      =  1
MSG_ID_SET_PUMP      =  2
MSG_ID_SET_SOLENOID  =  3
MSG_ID_SET_HEATER    =  4
UNUSED0              =  5
UNUSED1              =  6
UNUSED2              =  7
MSG_ID_GET_SWITCH    =  8
MSG_ID_GET_PRESSURE  =  9
MSG_ID_GET_WEIGHT    = 10
MSG_ID_GET_TEMP      = 11
MSG_ID_GET_AC_ON     = 12
UNUSED3              = 13
UNUSED4              = 14
UNUSED5              = 15

_SERIAL_PORT = "/dev/ttyS0"
_BAUDRATE = 115200
_TIMEOUT  = 0.01

_ser = serial.Serial(port=_SERIAL_PORT, baudrate = _BAUDRATE)
_header_decoder = bitstruct.compile('u4u4')

class DataPoint:
    """
    Attaches a timestamp to the value passed to the constructor. Access the value with
    data_point.val and the timestamp with data_point.t
    """
    def __init__(self, val) -> None:
        self.t = time()
        self.val = val

class Getter:
    """
    Abstract class to handles getting values over the uart_bridge. If the value was recently retrieved (not more than
    min_dwell_time seconds ago), then the value is just reused.
    """
    _last_reading : DataPoint = None

    def __init__(self, min_dwell_time : float, request_message, response_decoder) -> None:
        self._mindt = min_dwell_time
        self._msg = request_message
        self._decoder = response_decoder

    def read(self):
        if (self._last_reading) == None or (time.time() - self._last_reading.t > self._mindt):
            self._last_reading = DataPoint(self._decoder(_send_over_uart(self._msg, expect_response = True)))

class Setter:
    """
    Abstract class to handles setting values over the uart_bridge. If the value is unchanged from
    the last time it was set, nothing is sent
    """
    _last_setting : DataPoint = None

    def __init__(self, min_dwell_time : float, message_packer, message_id, message_len) -> None:
        self._mindt = min_dwell_time
        self._msg_packer = message_packer
        self._msg_id = message_id
        self._msg_len = message_len

    def write(self, val, force = False):
        # If forced or value has changed and min-dt has expired
        if (force or (val != self._last_setting.val and time.time() - self._last_setting.t > self._mindt)):
            _send_over_uart(self._msg_packer.pack(self._msg_id, self._msg_id, val), expect_response=False)
            self._last_setting = DataPoint(val)

def _send_over_uart(msg, expect_response = False):
    """
    Send the bytestring message, msg over the _SERIAL_PORT at the _BAUDRATE. If expect_response is
    True, then the function hangs until the full response is recieved.
    """
    # Clear serial port and write message
    _ser.read()
    _ser.write(msg)
    send_time = time.time()

    # If response is expected, wait until recieved and then return
    if expect_response:
        while(_ser.in_waiting == 0):
            if time.time() - send_time > _TIMEOUT:
                raise IOError("UART Timeout!")
        header = _header_decoder(_ser.read(1))
        while(_ser.in_waiting != header[1]):
            if time.time() - send_time > _TIMEOUT:
                raise IOError("UART Timeout!")
        return _ser.read(header[1])