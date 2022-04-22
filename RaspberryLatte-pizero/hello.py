import serial
import bitstruct
from time import sleep

def getPressure():
    msg_get_pressure = bitstruct.pack('u4u4', 5, 0)
    ser.write(msg_get_pressure)
    while(ser.in_waiting==0):
        pass
    sleep(0.01)
    response = bitstruct.unpack('u4u4u16', ser.read_all())
    print(f"Current pressure: {response[2]}")
    #print(bitstruct.unpack('u4u4u16', response))

def getSwitches():
    ser.write(bitstruct.pack('u4u4', 4, 0))
    while(ser.in_waiting==0):
        pass
    sleep(0.01)
    response = bitstruct.unpack('u4u4u8u8', ser.read_all())
    print(f"Switch status: {response[2]} : {response[3]}")

ser = serial.Serial(port="/dev/ttyS0", baudrate = 115200)
#getPressure()
getSwitches()
#while(True):
    # msg_id = int(input("Enter message ID\n> "))
    # msg_len = 1
    # msg_body = int(input("Enter message body value\n> "))
    # msg = bitstruct.pack('u4u4u8', msg_id, msg_len, msg_body)
    # print(msg)
    # ser.write(msg)