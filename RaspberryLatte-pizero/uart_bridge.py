import serial
import bitstruct
from time import sleep, time

ser = serial.Serial(port="/dev/ttyS0", baudrate = 115200)

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

def getPressure()->PressureReading:
    msg_get_pressure = bitstruct.pack('u4u4', 9, 0)
    ser.write(msg_get_pressure)
    while(ser.in_waiting==0):
        pass
    sleep(0.01)
    response = bitstruct.unpack('u4u4u16', ser.read_all())
    return PressureReading(response[2])

def getTempurature()->TempuratureReading:
    msg_get_temp = bitstruct.pack('u4u4', 11, 0)
    ser.write(msg_get_temp)
    while(ser.in_waiting==0):
        pass
    sleep(0.01)
    response = bitstruct.unpack('u4u4u16', ser.read_all())
    return TempuratureReading(response[2])

def getSwitches():
    ser.write(bitstruct.pack('u4u4', 8, 0))
    while(ser.in_waiting==0):
        pass
    sleep(0.01)
    response = bitstruct.unpack('u4u4u8u8', ser.read_all())
    return SwitchReading(response[2], response[3])



# def getWeight():
#     ser.write(bitstruct.pack('u4u4', 10, 0))
#     while(ser.in_waiting==0):
#         pass
#     sleep(0.01)
#     response = bitstruct.unpack('u4u4u24', ser.read_all())
#     print(f"Current weight (g): {-0.000152968191*response[2]+2491.937016352400}")

# def setHeater():
#     msg_id = int(input("Enter heater setting\n> "))
#     ser.write(bitstruct.pack('u4u4u8', 4, 1, msg_id))
