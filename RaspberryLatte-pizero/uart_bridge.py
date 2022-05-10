import serial
import bitstruct
import sys
from time import sleep, time

ser = serial.Serial(port="/dev/ttyS0", baudrate = 115200)

_MSG_ID_SET_GPIO      =  1
_MSG_ID_SET_PUMP      =  2
_MSG_ID_SET_SOLENOID  =  3
_MSG_ID_SET_HEATER    =  4

_MSG_ID_GET_SWITCH    =  8
_MSG_ID_GET_PRESSURE  =  9
_MSG_ID_GET_WEIGHT    = 10
_MSG_ID_GET_TEMP      = 11
_MSG_ID_GET_AC_ON     = 12

_MSG_LEN_GET_SWITCH   =  3
_MSG_LEN_GET_PRESSURE =  3
_MSG_LEN_GET_WEIGHT   =  4
_MSG_LEN_GET_TEMP     =  3
_MSG_LEN_GET_AC_ON    =  2

_get_pressure_msg = bitstruct.pack('u4u4', _MSG_ID_GET_PRESSURE, 0)
_get_temp_msg     = bitstruct.pack('u4u4', _MSG_ID_GET_TEMP, 0)
_get_switch_msg   = bitstruct.pack('u4u4', _MSG_ID_GET_SWITCH, 0)
_get_weight_msg   = bitstruct.pack('u4u4', _MSG_ID_GET_WEIGHT, 0)
_get_ac_on_msg    = bitstruct.pack('u4u4', _MSG_ID_GET_AC_ON, 0)

_decode_pressure_bs = bitstruct.compile('u4u4u16')
_decode_temp_bs     = bitstruct.compile('u4u4u16')
_decode_switches_bs = bitstruct.compile('u4u4u8u8')
_decode_weight_bs   = bitstruct.compile('u4u4u24')
_decode_ac_on_bs    = bitstruct.compile('u4u4u8')

_set_heater_bs      = bitstruct.compile('u4u4u8')
_set_pump_bs        = bitstruct.compile('u4u4u8')
_set_solenoid_bs    = bitstruct.compile('u4u4u8')
_set_1gpio_bs        = bitstruct.compile('u4u4u1u7')
_set_2gpio_bs        = bitstruct.compile('u4u4u1u7u1u7')
_set_3gpio_bs        = bitstruct.compile('u4u4u1u7u1u7u1u7')

class Reading:
    def __init__(self) -> None:
        self.timestamp = time()

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
    while(ser.in_waiting != _MSG_LEN_GET_PRESSURE):
        pass
    response = _decode_pressure_bs.unpack(ser.read_all())
    return PressureReading(response[2])

def get_tempurature()->TempuratureReading:
    ser.write(_get_temp_msg)
    while(ser.in_waiting != _MSG_LEN_GET_TEMP):
        pass
    response = _decode_temp_bs.unpack(ser.read_all())
    return TempuratureReading(response[2])

def get_switches()->SwitchReading:
    ser.write(_get_switch_msg)
    while(ser.in_waiting != _MSG_LEN_GET_SWITCH):
        pass
    response = _decode_switches_bs.unpack(ser.read_all())
    return SwitchReading(response[2], response[3])

def get_weight():
    ser.write(_get_weight_msg)
    while(ser.in_waiting != _MSG_LEN_GET_WEIGHT):
        pass
    response = _decode_weight_bs.unpack(ser.read_all())
    return ScaleReading(response[2])

def get_ac_on() -> bool:
    ser.write(_get_ac_on_msg)
    while(ser.in_waiting != _MSG_LEN_GET_AC_ON):
        pass
    response = _decode_ac_on_bs.unpack(ser.read_all())
    return (0!=response[2])

def set_heater_to(new_value):
    ser.write(_set_heater_bs.pack(_MSG_ID_SET_HEATER, 1, new_value))

def set_pump_to(new_value):
    ser.write(_set_pump_bs.pack(_MSG_ID_SET_PUMP, 1, new_value))

def set_solenoid_to(new_value):
    ser.write(_set_solenoid_bs.pack(_MSG_ID_SET_SOLENOID, 1, new_value))

def set_gpio_to(gpio_idx : int, val : bool):
    if gpio_idx is list:
        if len(gpio_idx) == 1:
            ser.write(_set_1gpio_bs.pack(_MSG_ID_SET_LEDS, 1, gpio_idx, val)
        elif len(gpio_idx) == 2:
            ser.write(_set_2gpio_bs.pack(_MSG_ID_SET_LEDS, 2, gpio_idx[0], val[0], gpio_idx[1], val[1])
        elif len(gpio_idx) == 3:
            ser.write(_set_3gpio_bs.pack(_MSG_ID_SET_LEDS, 3, gpio_idx[0], val[0], gpio_idx[1], val[1], gpio_idx[2], val[2])
        else:
            print("Can't send more than 3 gpio commands at a time")
    else:
        ser.write(_set_1gpio_bs.pack(_MSG_ID_SET_LEDS, 1, gpio_idx, val)
    

if __name__ == "__main__":
    cmd = sys.argv[1]
    if cmd=="solenoid":
        para = int(sys.argv[2])
        print(f"Setting solenoid to {para}")
        set_solenoid_to(para)
    elif cmd=="pump":
        para = int(sys.argv[2])
        print(f"Setting pump to {para}")
        set_pump_to(para)
    elif cmd=="heater":
        para = int(sys.argv[2])
        print(f"Setting heater to {para}")
        set_heater_to(para)
    elif cmd=="scale":
        print(f"Current weight is {get_weight().in_g()}")
    elif cmd=="switches":
        print(f"Current pump switch state is {get_switches().pump()}\nCurrent dial switch state is {get_switches().dial()}")
    elif cmd=="temp":
        print(f"Current temp is {get_tempurature().in_C()}")
    elif cmd=="ac_on":
        if (get_ac_on()):
            print("AC is on.")
        else:
            print("AC is off.")
    elif cmd=="pressure":
        print(f"Current pressure is {get_pressure().in_bar()} bar")
    