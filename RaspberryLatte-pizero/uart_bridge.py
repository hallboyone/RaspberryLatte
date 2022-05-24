import serial

_MSG_ID_GET_SWITCH    =  8
_MSG_ID_GET_PRESSURE  =  9
_MSG_ID_GET_WEIGHT    = 10
_MSG_ID_GET_TEMP      = 11
_MSG_ID_GET_AC_ON     = 12

_SERIAL_PORT = "/dev/ttyS0"
_BAUDRATE = 115200

_ser = serial.Serial(port=_SERIAL_PORT, baudrate = _BAUDRATE)
_header_decoder = bitstruct.compile('u4u4')

"""
Send the bytestring message, msg over the _SERIAL_PORT at the _BAUDRATE. If expect_response is
True, then the function hangs until the full response is recieved.
"""
def send(msg, expect_response = False):
    # Clear serial port and write message
    _ser.read()
    _ser.write(msg)

    # If response is expected, wait until recieved and then return
    if expect_response:
        while(_ser.in_waiting == 0):
            pass
        header = _header_decoder(_ser.read(1))
        while(_ser.in_waiting != header[1]):
            pass
        return _ser.read(header[1])


_MSG_ID_SET_LEDS      =  1
_MSG_ID_SET_PUMP      =  2
_MSG_ID_SET_SOLENOID  =  3
_MSG_ID_SET_HEATER    =  4


_set_heater_bs      = bitstruct.compile('u4u4u8')
_set_pump_bs        = bitstruct.compile('u4u4u8')
_set_solenoid_bs    = bitstruct.compile('u4u4u8')
_set_1gpio_bs        = bitstruct.compile('u4u4u1u7')
_set_2gpio_bs        = bitstruct.compile('u4u4u1u7u1u7')
_set_3gpio_bs        = bitstruct.compile('u4u4u1u7u1u7u1u7')

def set_heater_to(new_value):
    ser.write(_set_heater_bs.pack(_MSG_ID_SET_HEATER, 1, new_value))

def set_pump_to(new_value):
    ser.write(_set_pump_bs.pack(_MSG_ID_SET_PUMP, 1, new_value))

def set_solenoid_to(new_value):
    ser.write(_set_solenoid_bs.pack(_MSG_ID_SET_SOLENOID, 1, new_value))

def set_leds(led_num, val):
    if led_num is list:
        if len(led_num) == 1:
            ser.write(_set_1gpio_bs.pack(_MSG_ID_SET_LEDS, 1, val[0], led_num[0]))
        elif len(led_num) == 2:
            ser.write(_set_2gpio_bs.pack(_MSG_ID_SET_LEDS, 2, val[0], led_num[0], val[1], led_num[1]))
        elif len(led_num) == 3:
            ser.write(_set_3gpio_bs.pack(_MSG_ID_SET_LEDS, 3, val[0], led_num[0], val[1], led_num[1], val[2], led_num[2]))
        else:
            print("Can't send more than 3 gpio commands at a time")
    else:
        ser.write(_set_1gpio_bs.pack(_MSG_ID_SET_LEDS, 1, val, led_num))
    

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
    elif cmd=="leds":
        led_num = int(sys.argv[2])
        val = int(sys.argv[3])
        set_leds(led_num, val)
    