cmake_minimum_required(VERSION 3.12)

set(PICO_BOARD pico_w)

include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

project(RaspberryLatte C CXX ASM)
set(CMAKE_C_STANDARD 11)

set(PICO_BOARD pico_w)
pico_sdk_init()

add_subdirectory(firmware)