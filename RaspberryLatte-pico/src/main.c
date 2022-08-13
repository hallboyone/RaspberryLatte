#include <stdio.h>

#include "espresso_machine.h"
#include "loop_rate_limiter.h"

int main(){
    // Setup UART
    stdio_uart_init_full(PICO_DEFAULT_UART_INSTANCE, 115200, PICO_DEFAULT_UART_TX_PIN, PICO_DEFAULT_UART_RX_PIN);

    if(espresso_machine_setup()){
        return 1;
    }

    // Run main machine loop
    uint loop_counter = 0;
    uint64_t last_loop_time_us = time_us_64();
    while(true){
        espresso_machine_tick();

        loop_rate_limiter_us(&last_loop_time_us, 10000);
    }
}