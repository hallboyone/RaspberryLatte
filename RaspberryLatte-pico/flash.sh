#!/bin/bash
openocd -f interface/raspberrypi-swd.cfg -f target/rp2040.cfg -c "program ../build/RaspberryLatte-pico/RaspberryLattePico.elf verify reset exit"