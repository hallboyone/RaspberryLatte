# Information from http://derekmolloy.ie/hello-world-introductions-to-cmake/

cmake_minimum_required(VERSION 3.12)

include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

project(RaspberryLattePico C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

include_directories(include)

add_executable(RaspberryLattePico src/main.c)

pico_generate_pio_header(RaspberryLattePico ${CMAKE_CURRENT_LIST_DIR}/src/components/hx711.pio)
pico_generate_pio_header(RaspberryLattePico ${CMAKE_CURRENT_LIST_DIR}/src/components/lmt01.pio)

target_sources(RaspberryLattePico 
               PRIVATE ./src/main.c 
               PRIVATE src/components/phasecontrol.c
               PRIVATE src/components/physical_inputs.c
               PRIVATE src/components/packer.c)

target_link_libraries(RaspberryLattePico PRIVATE 
  pico_stdlib
  hardware_adc
  pico_time
  hardware_pio
  pico_multicore)

pico_add_extra_outputs(RaspberryLattePico)