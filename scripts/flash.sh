#!/bin/bash
python /home/hallboyone/RaspberryLatte-pico/scripts/reset_pico.py
sleep 0.5s
openocd -f interface/raspberrypi-swd.cfg -f target/rp2040.cfg -c "program /home/hallboyone/RaspberryLatte-pico/build/pico_firmware/RaspberryLattePico.elf verify reset exit"