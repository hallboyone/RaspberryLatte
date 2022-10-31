#include <stdio.h>

#include "espresso_machine.h"
#include "loop_rate_limiter.h"

#include "machine_settings.h"
#include "pinout.h"

int main(){
    // Setup UART
    stdio_uart_init_full(PICO_DEFAULT_UART_INSTANCE, 115200, PICO_DEFAULT_UART_TX_PIN, PICO_DEFAULT_UART_RX_PIN);

    i2c_inst_t *  bus = i2c1;
    i2c_bus_setup(bus, 100000, I2C_SCL_PIN, I2C_SDA_PIN);

    mb85_fram mem;
    mb85_fram_setup(&mem, bus, 0x00, NULL);
    
    machine_settings settings = machine_settings_setup(& mem);
    machine_settings_print();
    machine_settings_set(MACHINE_SETTING_TEMP_BREW_DC, 920);
    machine_settings_print();
    machine_settings_save_profile(1);
    machine_settings_set(MACHINE_SETTING_TEMP_BREW_DC, 890);
    machine_settings_print();
    machine_settings_load_profile(1);
    machine_settings_print();
    return 0;
}