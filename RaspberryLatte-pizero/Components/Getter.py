import uart_bridge
import time

class Response:
    def __init__(self, body) -> None:
        self.timestamp = time()
        self.body = body

class Getter:
    """
    Abstract class to handles getting values over the uart_bridge. If the value was recently retrieved (not more than
    min_dwell_time seconds ago), then the value is just reused.
    """
    _last_reading : Response = None

    _getter_msg = None
    _get_response_len = None

    def __init__(self, min_dwell_time : float, request_message, response_decoder) -> None:
        super().__init__()
        self._min_dwell_time = min_dwell_time
        self._msg = request_message
        self._decoder = response_decoder

    def read(self):
        if (self._last_reading) == None or (time.time() - self._last_reading.timestamp > self._min_dwell_time):
            self._last_reading = Response(self._decoder(uart_bridge.send(self._msg, expect_response = True)))