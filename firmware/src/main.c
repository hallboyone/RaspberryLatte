#include <stdio.h>

#include "machine_logic/espresso_machine.h"
#include "machine_logic/machine_settings.h"
#include "pinout.h"

#define S_TO_US 1000000
#define MS_TO_US 1000

int main(){
    // Setup UART
    stdio_uart_init_full(PICO_DEFAULT_UART_INSTANCE, 115200, PICO_DEFAULT_UART_TX_PIN, PICO_DEFAULT_UART_RX_PIN);

    espresso_machine_viewer espresso_machine;
    if(espresso_machine_setup(&espresso_machine)){
        return 1;
    }
    machine_settings * settings = machine_settings_acquire();

    // Message rate limiter
    absolute_time_t next_msg_time = get_absolute_time();
    const uint64_t msg_period_us = 200 * MS_TO_US;

    // Loop rate limiter
    absolute_time_t next_loop_time;
    const uint64_t loop_period_us = 10 * MS_TO_US;

    while(true){
        next_loop_time = make_timeout_time_us(loop_period_us);

        // Update the machine
        espresso_machine_tick();
        if(espresso_machine->pump.flowrate_ml_s < 0){
            printf("Warning\n");
        }
        // Print status once a second
        if(absolute_time_diff_us(get_absolute_time(), next_msg_time) < 0){
            next_msg_time = make_timeout_time_us(msg_period_us);
            if(espresso_machine->switches.ac_switch){
                const uint32_t timestamp_ms = to_ms_since_boot(get_absolute_time());
                printf("%07d,%0.2f,%0.2f,%03d,%0.3f,%0.3f\n",
                timestamp_ms,
                espresso_machine->boiler.setpoint/16.,
                espresso_machine->boiler.temperature/16.,
                espresso_machine->pump.power_level,
                espresso_machine->pump.flowrate_ml_s,
                espresso_machine->pump.pressure_bar);
            }
        }
        sleep_until(next_loop_time);
    }
}