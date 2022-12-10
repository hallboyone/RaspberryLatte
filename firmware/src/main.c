#include <stdio.h>

#include "espresso_machine.h"

#include "machine_settings.h"
#include "pinout.h"

int main(){
    // Setup UART
    stdio_uart_init_full(PICO_DEFAULT_UART_INSTANCE, 115200, PICO_DEFAULT_UART_TX_PIN, PICO_DEFAULT_UART_RX_PIN);

    espresso_machine_viewer espresso_machine;
    if(espresso_machine_setup(&espresso_machine)){
        return 1;
    }

    // Run main machine loop
    absolute_time_t next_msg_time = get_absolute_time();
    const uint64_t msg_period_us = 1000000;
    absolute_time_t next_loop_time;
    const uint64_t loop_period_us = 10000;
    
    while(true){
        next_loop_time = make_timeout_time_us(loop_period_us);
        espresso_machine_tick();
        
        if(absolute_time_diff_us(get_absolute_time(), next_msg_time) < 0){
            next_msg_time = make_timeout_time_us(loop_period_us);
            if(espresso_machine->boiler.setpoint){
                printf("T = %0.2f / %0.2f\n", espresso_machine->boiler.temperature/16., espresso_machine->boiler.setpoint/16.);
            }
        }

        sleep_until(next_loop_time);
    }
}