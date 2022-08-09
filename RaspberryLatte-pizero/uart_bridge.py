import serial
import bitstruct
from time import time, sleep

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
MSG_ID_GET_DIAL      = 13
UNUSED4              = 14
UNUSED5              = 15

_SERIAL_PORT = "/dev/ttyS0"
_BAUDRATE = 115200
UART_TIMEOUT  = 0.25
RESEND_COUNT = 3

_ser = serial.Serial(port=_SERIAL_PORT, baudrate = _BAUDRATE)
_header_encoding = bitstruct.compile('u8u8')
_header_status_encoding = bitstruct.compile('u8u8u8')
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
    Object for communicating over UART bridge. Every messanger contains
    - A message ID
    - An optional minimum time between sends
    - An optional ability to repeat consequtives sends of the same values

    The send method of this class does the following things
    - Takes an optional bytes parameter which is the message body
    - Computes the length of body, len. 
    - Packs the binary message into [0-3:ID][4-8:len]-[9-:Body(opt)]
    - Send message over UART and waits for response
    - Reads first two bytes and unpacks into a response ID, response len, and response status
    - Verifies the response ID matches the message ID
    - Reads the number of bytes specified by the response len into the raw_body property
    - Returns the status
    """

    def __init__(self, msg_id : int, min_dwell_time : float = 0.0, avoid_repeat_sends = False):
        self.msg_id = msg_id

        self.mindt       = min_dwell_time
        self._last_msg_t = 0.0

        self.avoid_repeat_sends = avoid_repeat_sends
        self.prev_body : bytes  = None

        self.status : int       = None
        self.response : bytes   = None

    def send(self, msg_body : bytes = None, force = False) -> int:
        """ 
        Send the msg over UART, wait for response, and save status and response body into self.status
        and self.respsonse respectively. If no response is recieved within UART_TIMEOUT, an exception
        is thrown. 

        Parameters:
        - msg_body  : bytes - Message to send over UART. 
        - force : bool  - Send msg regardless of dwell time and duplicate checks (default = False) 

        Returns:
        - Interger representing the status of the response.
        """
        dwell_time_expired = time() - self._last_msg_t > self.mindt
        non_duplicate = (not self.avoid_repeat_sends or self.prev_body is None or self.prev_body != msg_body)
        if (force or (dwell_time_expired and non_duplicate)):
            for i in range(RESEND_COUNT):
                try:
                    self._send(msg_body, force)
                except Exception as e:
                    if i < RESEND_COUNT:
                        print(f"Failure {i}/{RESEND_COUNT}: {e}\nTrying again...")
                        sleep(0.1)
                    else:
                        raise e
                else:
                    break
        return self.status

    def _send(self, msg_body : bytes, force : bool):
        if msg_body is None:
            body_len = 0
        else:
            body_len = len(msg_body)
        
        # Clear input buffer and write message
        _ser.reset_input_buffer()
        _ser.write(_header_encoding.pack(self.msg_id, body_len))
        if msg_body is not None:
            _ser.write(msg_body)
        self._last_msg_t = time()

        # Read and unpack header and status bytes
        while(_ser.in_waiting < 3):
            if time() - self._last_msg_t > UART_TIMEOUT:
                raise IOError("UART Timeout! Timeout occured while waiting for header")
        
        (response_id, response_body_len, self.status) = _header_status_encoding.unpack(_ser.read(3))
        if response_id != self.msg_id:
            raise IOError(f"Invalid reponse: message IDs don't match ({self.msg_id} vs. {response_id}x{response_body_len}x{self.status})")

        # Read body
        while(_ser.in_waiting < response_body_len):
            if time() - self._last_msg_t > UART_TIMEOUT:
                raise IOError("UART Timeout! Timeout occured while waiting for body")
        self.response : bytes = _ser.read(response_body_len)

        # Create deep copy of message for next time
        if self.avoid_repeat_sends:
            if msg_body is None:
                self.prev_msg = None
            else:
                self.prev_msg = bytearray(len(msg_body))
                self.prev_msg[:] = msg_body

def clearUART():
    _ser.read_all()
