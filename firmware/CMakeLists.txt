include_directories(include)

add_executable(RaspberryLattePico src/main.c)

pico_generate_pio_header(RaspberryLattePico ${CMAKE_CURRENT_LIST_DIR}/src/drivers/lmt01.pio)

target_sources(RaspberryLattePico
               PRIVATE ./src/main.c
               PRIVATE src/machine_logic/machine_settings.c
               PRIVATE src/machine_logic/espresso_machine.c
               PRIVATE src/machine_logic/local_ui.c
               PRIVATE src/machine_logic/autobrew.c
               PRIVATE src/machine_logic/ulka_pump.c
               PRIVATE src/drivers/nau7802.c
               PRIVATE src/drivers/flow_meter.c
               PRIVATE src/drivers/lmt01.c
               PRIVATE src/drivers/mb85_fram.c
               PRIVATE src/utils/slow_pwm.c
               PRIVATE src/utils/pid.c
               PRIVATE src/utils/i2c_bus.c
               PRIVATE src/utils/value_flasher.c
               PRIVATE src/utils/gpio_multi_callback.c
               PRIVATE src/utils/analog_input.c
               PRIVATE src/utils/binary_output.c
               PRIVATE src/utils/binary_input.c
               PRIVATE src/utils/phasecontrol.c
               PRIVATE src/utils/gpio_irq_timestamp.c)

target_link_libraries(RaspberryLattePico PRIVATE 
  pico_stdlib
  hardware_adc
  hardware_pio
  hardware_i2c)
