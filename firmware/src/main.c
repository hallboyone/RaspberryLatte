#include <stdio.h>

#include "espresso_machine.h"
#include "loop_rate_limiter.h"

#include "machine_settings.h"
#include "pinout.h"

#include "gpio_multi_callback.h"

uint32_t num_trigger_1 = 0;
uint32_t num_trigger_2 = 0;

void cb_1(uint8_t gpio, uint32_t event, void* data){
    uint32_t* num = (uint32_t*)data;
    *num += 1;
}

void cb_1(uint8_t gpio, uint32_t event, void* data){
    uint32_t* num = (uint32_t*)data;
    *num += 2;
}

int main(){
    // Setup UART
    stdio_uart_init_full(PICO_DEFAULT_UART_INSTANCE, 115200, PICO_DEFAULT_UART_TX_PIN, PICO_DEFAULT_UART_RX_PIN);

    
    // espresso_machine_viewer espresso_machine;
    // if(espresso_machine_setup(&espresso_machine)){
    //     return 1;
    // }

    // // Run main machine loop
    // uint64_t last_msg_time = time_us_64();
    // uint64_t last_loop_time_us = time_us_64();
    
    // while(true){
    //     espresso_machine_tick();

    //     if(last_msg_time + 1000000 <= time_us_64()){
    //         last_msg_time += 1000000;
    //         if(espresso_machine->boiler.setpoint){
    //             printf("T = %0.2f / %0.2f\n", espresso_machine->boiler.temperature/16., espresso_machine->boiler.setpoint/16.);
    //         }
    //     }
    //     loop_rate_limiter_us(&last_loop_time_us, 10000);
    // }
}