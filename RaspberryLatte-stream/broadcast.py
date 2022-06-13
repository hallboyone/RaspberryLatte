import socket
from time import sleep

UDP_IP = "192.168.0.126"
UDP_PORT = 5005

f = open("../brew_1.txt", 'r')
lines = f.readlines()
f.close()
MESSAGE = "t=1655086881.5652995;temp=50.25;scale=-6.093334920294001;pressure=4.095;pump=0;heater=2.071128030594438;stage=0;"

with socket.socket( socket.AF_INET, socket.SOCK_DGRAM ) as s:
    for ln in lines:
        s.sendto(ln.encode(), (UDP_IP, UDP_PORT))
        sleep(0.05)
    s.close()