# RaspberryLatte

A full stack suite to add smarts to a single boiler espresso machine using a Raspberry Pi Pico as the brains. 

## Description

This project can be thought of in several layers. 
### Hardware Level
The lowest level is the hardware such as sensors and switches required to create a smart espresso machine. Additionally, a custom carrier board is needed to interface with these components. This PCB contains any circuitry required to support the sensors and switches. Mounted to this carrier board is the Raspberry Pi Pico W. 
![Overhead view of main board](https://github.com/hallboyone/RaspberryLatte/blob/master/docs/RaspberryLatte_main_board.png?raw=true)

### Firmware Level
The next level consists of the firmware running on the Pico. This code, written in c, contains all the sensor drivers and machine logic. The firmware is partitioned onto a single core of the dual core Pico.
### Network Level (Planned)
The network level runs on the second core on the Pico. It collects information from the firmware running on core 0 and shares that over BLE with the Mobile UI level (see below). It also passes settings received from a BLE client to the firmware so that the machines performance can be tailored to achieve the desired brew characteristics. NOTE: This cannot be implemented until the pico SDK add BLE support.
### Mobile UI Level (Planned)
While ssh is fun, a mobile application would be significantly more convenient. This level would allow users to create and share their brew profiles for different coffees, visualize a shots flow rate, pressure, temperature, etc. in real time, and troubleshoot their shots. This application would interface with the Pi 0w over BLE.

## To-Do
- [X] Finish Software Level
- [x] Combine Firmware and Software levels to run together on the Pi pico W (eliminates the Pi 0w)
- [ ] Add carrier board design to repository
- [ ] Add wiring diagram
- [ ] Add BOM for hardware
- [ ] Implement Network level
- [ ] Implement mobile UI level

## Inspiration
This project has taken inspiration from others who have taken the time to write up their own builds. The two primary sources have been the [Es(pi)resso project](https://home-automations.net/project-coffee-espiresso-machine/), and the [Espresso for Geeks blog](https://www.instructables.com/Espresso-for-Geeks/). Big thank you to both those makers for sharing the great work they did!

## Contribute
If you are interested in helping with this project, I would love to talk! You can reach me through [Twitter](https://twitter.com/hallboyone "Richard Hall").
