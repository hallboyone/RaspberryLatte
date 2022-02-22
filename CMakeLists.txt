cmake_minimum_required(VERSION 3.12)

include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)
pico_sdk_init()

project(RaspberryLatte-pico)

#add_executable(phasecontrol ./src/phasecontrol.)c
add_executable(main)

#pico_generate_pio_header(main ${CMAKE_CURRENT_LIST_DIR}/src/components/hx711.pio)
pico_generate_pio_header(main ${CMAKE_CURRENT_LIST_DIR}/src/components/lmt01.pio)

target_sources(main PRIVATE ./src/main.c)

target_link_libraries(main PRIVATE pico_stdlib
  hardware_pwm
  hardware_uart
  pico_time
  hardware_pio
  hardware_spi
  pico_multicore)

pico_add_extra_outputs(main)
