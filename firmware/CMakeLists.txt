include_directories(include)

add_executable(RaspberryLattePico src/main.c)

pico_generate_pio_header(RaspberryLattePico ${CMAKE_CURRENT_LIST_DIR}/src/components/lmt01.pio)

target_sources(RaspberryLattePico
               PRIVATE ./src/main.c
               PRIVATE src/machine_settings.c
               PRIVATE src/espresso_machine.c
               PRIVATE src/components/analog_input.c
               PRIVATE src/components/binary_output.c
               PRIVATE src/components/binary_input.c
               PRIVATE src/components/phasecontrol.c
               PRIVATE src/components/nau7802.c
               PRIVATE src/components/slow_pwm.c
               PRIVATE src/components/lmt01.c
               PRIVATE src/components/pid.c
               PRIVATE src/components/autobrew.c
               PRIVATE src/espresso_machine.c
               PRIVATE src/components/loop_rate_limiter.c
               PRIVATE src/components/i2c_bus.c
               PRIVATE src/components/value_flasher.c
               PRIVATE src/components/mb85_fram.c)

target_link_libraries(RaspberryLattePico PRIVATE 
  pico_stdlib
  hardware_adc
  hardware_pio
  hardware_i2c)
