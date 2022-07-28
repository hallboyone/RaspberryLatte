"""
Provides getter and setter classes for retreiving sensor data and sending commands over uart
to pico running RaspberryLatte firmware. The module should be primarly used by inheriting
the Getter and Setter classes which internally call the _send_over_uart function.
"""
import serial
import bitstruct
import time

import status_ids

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
UART_TIMEOUT  = 0.2

_ser = serial.Serial(port=_SERIAL_PORT, baudrate = _BAUDRATE)
_header_decoder = bitstruct.compile('u4u4')
_header_status_decoder = bitstruct.compile('u4u4u8')
class DataPoint:
    """
    Attaches a timestamp to the value passed to the constructor. Access the value with
    data_point.val and the timestamp with data_point.t
    """
    def __init__(self, val) -> None:
        self.t = time.time()
        self.val = val

class UARTMessenger:
    """
    Object for communicating over UART bridge. The object contains optional restrictions on the minimum time between 
    messages and avoiding repeat messages. Each message is a bytes or bytearray object where the highest four bits of
    the first byte correspond to the message's id, the lowest four correspond with the body length (not including first
    byte), and the next (optional) bytes are the message's body. After a message is sent, a response is expected 
    consisting of the same message ID, body length, status code, and body (optional). The status and body (if exists)
    are stored as variables until the next call.
    """

    def __init__(self, min_dwell_time : float = 0.0, avoid_repeat_sends = False):
        self._last_msg_t = 0.0
        self.mindt = min_dwell_time

        self.avoid_repeat_sends = avoid_repeat_sends
        self.prev_msg = None
        self.status : int = None
        self.response : bytes = None

    def send(self, msg : bytes, force = False):
        """ 
        Send the msg over UART, wait for response, and save status and response body into self.status
        and self.respsonse respectively. If no response is recieved within UART_TIMEOUT, an exception
        is thrown. 

        Parameters:
        - msg   : bytes - Message to send over UART. Should be formated as [[7-4 ID][3-0 len]][body0][body1]...
        - force : bool  - Send msg regardless of dwell time and duplicate checks (default = False) 
        """
        dwell_time_up = time.time() - self._last_msg_t > self.mindt
        non_duplicate = (not self.avoid_repeat_sends or self.prev_msg==None or self.prev_msg!=msg)
        if (force or (dwell_time_up and non_duplicate)):
            # Clear input and write message
            _ser.reset_input_buffer()
            _ser.write(msg)
            self._last_msg_t = time.time()

            # Read and unpack header
            while(_ser.in_waiting < 2):
                if time.time() - self._last_msg_t > UART_TIMEOUT:
                    raise IOError("UART Timeout!")
            body_len : int
            (_, body_len, self.status) = _header_status_decoder.unpack(_ser.read(2))

            # Read body
            while(_ser.in_waiting < body_len):
                if time.time() - self._last_msg_t > UART_TIMEOUT:
                    raise IOError("UART Timeout!")
            self.response : bytes = _ser.read(body_len)

            # Create deep copy of message for next time
            if self.avoid_repeat_sends:
                self.prev_msg = bytearray(len(msg))
                self.prev_msg[:] = msg
        
class Getter(UARTMessenger):
    """
    TODO: write documentation
    """
    def __init__(self, min_dwell_time : float, avoid_repeat_sends, request_message, response_decoder) -> None:
        UARTMessenger.__init__(self, min_dwell_time, avoid_repeat_sends)
        self._mindt = min_dwell_time
        self.request_msg = request_message
        self.request_decoder = response_decoder

    def read(self) -> int:
        UARTMessenger.send(self, self.request_msg)
        if self.status == status_ids.SUCCESS:
            self.unpacked_response = self.request_decoder.unpack(self.response)
        return self.status

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
        if force or (self._last_setting is None) or (val != self._last_setting.val and time.time() - self._last_setting.t > self._mindt):
            _send_over_uart(self._msg_packer.pack(self._msg_id, self._msg_len, val), expect_response=False)
            self._last_setting = DataPoint(val)

def _send_over_uart(msg, expect_response = False):
    """
    Send the bytestring message, msg over the _SERIAL_PORT at the _BAUDRATE. If expect_response is
    True, then the function hangs until the full response is recieved.
    """
    # Clear serial port and write message
    _ser.read(_ser.in_waiting)
    _ser.write(msg)
    send_time = time.time()

    # If response is expected, wait until recieved and then return
    if expect_response:
        while(_ser.in_waiting == 0):
            if time.time() - send_time > UART_TIMEOUT:
                raise IOError("UART Timeout!")
        header = _header_decoder.unpack(_ser.read(1))
        while(_ser.in_waiting != header[1]):
            if time.time() - send_time > UART_TIMEOUT:
                raise IOError("UART Timeout!")
        return _ser.read(header[1])

def clearUART():
    _ser.read_all()