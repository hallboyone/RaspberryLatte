import socket
from time import sleep

UDP_IP = "192.168.0.126"
UDP_PORT = 5005

f = open("2022-6-12-21:36:26.brewlog", 'r')
lines = f.readlines()
f.close()

with socket.socket( socket.AF_INET, socket.SOCK_DGRAM ) as s:
    for ln in lines:
        s.sendto(ln.encode(), (UDP_IP, UDP_PORT))
        sleep(0.05)
    s.close()