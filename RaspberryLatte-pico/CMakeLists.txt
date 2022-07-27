include_directories(include)

add_executable(RaspberryLattePico src/main.c)

pico_generate_pio_header(RaspberryLattePico ${CMAKE_CURRENT_LIST_DIR}/src/components/hx711.pio)
pico_generate_pio_header(RaspberryLattePico ${CMAKE_CURRENT_LIST_DIR}/src/components/lmt01.pio)

target_sources(RaspberryLattePico 
               PRIVATE ./src/main.c 
               PRIVATE src/components/pid.c)

target_link_libraries(RaspberryLattePico PRIVATE 
  pico_stdlib
  pico_multicore
  hardware_adc
  hardware_pio
  hardware_i2c) 


#pico_add_extra_outputs(RaspberryLattePico)