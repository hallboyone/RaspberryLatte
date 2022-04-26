include_directories(include)

add_executable(RaspberryLattePico src/main.c)

pico_generate_pio_header(RaspberryLattePico ${CMAKE_CURRENT_LIST_DIR}/src/components/hx711.pio)
#pico_generate_pio_header(RaspberryLattePico ${CMAKE_CURRENT_LIST_DIR}/src/components/lmt01.pio)

target_sources(RaspberryLattePico 
               PRIVATE ./src/main.c 
               PRIVATE src/components/uart_bridge.c
               PRIVATE src/components/pressure_sensor.c
               PRIVATE src/components/binary_input.c
               PRIVATE src/components/phasecontrol.c
               PRIVATE src/components/hx711.c
               PRIVATE src/components/maintainer.c)
               #PRIVATE src/components/uart_bridge.c)

target_link_libraries(RaspberryLattePico PRIVATE 
  pico_stdlib
  pico_multicore
  hardware_adc
  hardware_pio)
  #pico_time
  #hardware_pio
  #pico_multicore)


#pico_add_extra_outputs(RaspberryLattePico)