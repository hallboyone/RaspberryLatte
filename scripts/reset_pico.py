import RPi.GPIO as GPIO
from time import sleep
print("Reseting... ", end = "")
GPIO.setmode(GPIO.BCM)
GPIO.setup(16, GPIO.OUT)
GPIO.output(16, 1)
sleep(0.01)
GPIO.output(16, 0)
GPIO.cleanup()
print("Done!")