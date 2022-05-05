import serial
import bitstruct
from time import sleep, time

ser = serial.Serial(port="/dev/ttyS0", baudrate = 115200)

_MSG_ID_SET_LEDS      =  1
_MSG_ID_SET_PUMP      =  2
_MSG_ID_SET_SOLENOID  =  3
_MSG_ID_SET_HEATER    =  4

_MSG_ID_GET_SWITCH    =  8
_MSG_ID_GET_PRESSURE  =  9
_MSG_ID_GET_WEIGHT    = 10
_MSG_ID_GET_TEMP      = 11
_MSG_ID_GET_AC_ON     = 12

_get_pressure_msg = bitstruct.pack('u4u4', _MSG_ID_GET_PRESSURE, 0)
_get_temp_msg     = bitstruct.pack('u4u4', _MSG_ID_GET_TEMP, 0)
_get_switch_msg   = bitstruct.pack('u4u4', _MSG_ID_GET_SWITCH, 0)
_get_weight_msg   = bitstruct.pack('u4u4', _MSG_ID_GET_WEIGHT, 0))

_decode_pressure_bs = bitstruct.compile('u4u4u16')
_decode_temp_bs     = bitstruct.compile('u4u4u16')
_decode_switches_bs = bitstruct.compile('u4u4u8u8')
_decode_weight_bs   = bitstruct.compile('u4u4u24')

_set_heater_bs      = bitstruct.compile('u4u4u8')

class Reading:
    def __init__(self) -> None:
        self._timestamp = time()

class PressureReading(Reading):
    def __init__(self, value) -> None:
        super().__init__()
        self._raw_val = value
    
    def in_bar(self)->float:
        return self._raw_val/1000.0 

class TempuratureReading(Reading):
    def __init__(self, value) -> None:
        super().__init__()
        self._raw_val = value
    
    def in_C(self)->float:
        return self._raw_val/16.0
    
    def in_F(self)->float:
        return self.in_C()*1.8+32

class SwitchReading(Reading):
    def __init__(self, pump_val, dial_val) -> None:
        super().__init__()
        self._pump_val = pump_val
        self._dial_val = dial_val
    
    def pump(self)->bool:
        return self._pump_val
    
    def dial(self)->int:
        return self._dial_val

class ScaleReading(Reading):
    def __init__(self, raw_val) -> None:
        super().__init__()
        self._raw_val = raw_val
    
    def in_g(self)->float:
        return -0.000152968191*self._raw_val+2491.937016352400

def get_pressure()->PressureReading:
    ser.write(_get_pressure_msg)
    while(ser.in_waiting==0):
        pass
    sleep(0.005)
    response = _decode_pressure_bs.unpack(ser.read_all())
    return PressureReading(response[2])

def get_tempurature()->TempuratureReading:
    ser.write(_get_temp_msg)
    while(ser.in_waiting==0):
        pass
    sleep(0.005)
    response = _decode_temp_bs.unpack(ser.read_all())
    return TempuratureReading(response[2])

def get_switches()->SwitchReading:
    ser.write(_get_switch_msg)
    while(ser.in_waiting==0):
        pass
    sleep(0.005)
    response = _decode_switches_bs.unpack(ser.read_all())
    return SwitchReading(response[2], response[3])

def get_weight():
    ser.write(_get_weight_msg)
    while(ser.in_waiting==0):
        pass
    sleep(0.005)
    response = _decode_weight_bs.unpack(ser.read_all())
    return ScaleReading(response[2])

def set_heater_to(int: new_value):
    ser.write(_set_heater_bs.pack(_MSG_ID_SET_HEATER, 1, new_value))
