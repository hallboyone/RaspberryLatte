#define PRINT_MACHINE_STATUS_OVER_UART

#ifdef PRINT_MACHINE_STATUS_OVER_UART
#include <stdio.h>
#endif

#include "machine_logic/espresso_machine.h"

int main(){
    // Setup UART
    stdio_uart_init_full(PICO_DEFAULT_UART_INSTANCE, 115200, PICO_DEFAULT_UART_TX_PIN, PICO_DEFAULT_UART_RX_PIN);

    espresso_machine_viewer espresso_machine;
    if(espresso_machine_setup(&espresso_machine)) return 1;

    // Loop rate limiter
    absolute_time_t next_loop_time = get_absolute_time();
    const uint64_t loop_period_ms = 10;

    #ifdef PRINT_MACHINE_STATUS_OVER_UART
    const uint16_t ticks_per_message = 20;
    uint32_t num_ticks = 0;
    #endif

    while(true){
        next_loop_time = delayed_by_ms(next_loop_time, loop_period_ms);

        #ifdef PRINT_MACHINE_STATUS_OVER_UART
        num_ticks += 1;
        // Print status periodically 
        if(num_ticks%ticks_per_message == 0){
            if(espresso_machine->switches.ac_switch){
                const uint32_t timestamp_ms = to_ms_since_boot(get_absolute_time());
                printf("%07d,%0.2f,%0.2f,%d,%03d,%0.3f,%0.3f\n",
                timestamp_ms,
                espresso_machine->boiler.setpoint/16.,
                espresso_machine->boiler.temperature/16.,
                espresso_machine->boiler.power_level,
                espresso_machine->pump.power_level,
                espresso_machine->pump.flowrate_ml_s,
                espresso_machine->pump.pressure_bar);
            }
        }
        #endif

        // Update the machine
        espresso_machine_tick();

        sleep_until(next_loop_time);
    }
}