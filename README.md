# RaspberryLatte

A full stack suite to add smarts to a single boiler espresso machine using a Raspberry Pi Pico as the brains. 

## Description

This project can be thought of in several layers. 
### Hardware Level
The lowest level is the hardware such as sensors and switches required to create a smart espresso machine. Additionally, a custom carrier board is needed to interface with these components. This PCB contains any circuitry required to support the sensors and switches. Mounted to this carrier board is the Raspberry Pi Pico W. 
### Firmware Level
The next level consists of the firmware running on the Pico. This code, written in c, acts as a bridge between the fast time scale hardware level, and the slower software level (see below). The firmware begins by setting up each component, and then continually listens for UART messages from the software. When a message is read, the appropriate handler is called and a response is returned over UART.
### Software Level
Coordinating that actual control of the espresso machine is the software level. This level is written in python and runs on a Raspberry Pi 0w. The software retrieves sensor data from the firmware level over a UART bridge, updates the machine's state, and sends commands to the firmware to pass on to the machine inputs (such as the boiler, pump, etc.). In the future, the software level will also interface with the mobile UI level through BLE.
### Mobile UI Level (Planned)
While ssh is fun, a mobile application would be significantly more convenient. This level would allow users to create and share their brew profiles for different coffees, visualize a shots flow rate, pressure, temperature, etc. in real time, and troubleshoot their shots. This application would interface with the Pi 0w over BLE.

## To-Do
- [ ] Finish Software Level
- [ ] Combine Firmware and Software levels to run together on the Pi pico W (eliminates the Pi 0w)
- [ ] Add carrier board design to repo
- [ ] Add wiring diagram
- [ ] Add BOM for hardware
- [ ] Implement mobile UI level

## Contribute
If you are interested in helping with this project, I would love to talk! You can reach me through [Twitter](https://twitter.com/hallboyone "Richard Hall").