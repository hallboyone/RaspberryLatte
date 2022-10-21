// #include <stdio.h>

// #include "espresso_machine.h"
// #include "loop_rate_limiter.h"

// int main(){
//     // Setup UART
//     stdio_uart_init_full(PICO_DEFAULT_UART_INSTANCE, 115200, PICO_DEFAULT_UART_TX_PIN, PICO_DEFAULT_UART_RX_PIN);

//     espresso_machine_viewer espresso_machine;
//     if(espresso_machine_setup(&espresso_machine)){
//         return 1;
//     }

//     // Run main machine loop
//     uint64_t last_msg_time = time_us_64();
//     uint64_t last_loop_time_us = time_us_64();
    
//     while(true){
//         espresso_machine_tick();

//         if(last_msg_time + 500000 <= time_us_64()){
//             last_msg_time += 500000;
//             printf("T = %0.2f / %0.2f\n", espresso_machine->boiler.tempurature/16., espresso_machine->boiler.setpoint/16.);
//             // printf("Scale = %0.2f / %0.2f\n\n", espresso_machine->scale.val_mg/1000., espresso_machine->scale.flowrate_mg_s);
//             printf("%0.2f\n", espresso_machine->scale.flowrate_mg_s);
//         }
//         loop_rate_limiter_us(&last_loop_time_us, 10000);
//     }
// }

#include <stdio.h>
#include "value_flasher.h"
#include "pinout.h"
#include "binary_output.h"

int main(){
    stdio_uart_init_full(PICO_DEFAULT_UART_INSTANCE, 115200, PICO_DEFAULT_UART_TX_PIN, PICO_DEFAULT_UART_RX_PIN);

    binary_output leds;
    const uint8_t led_pins[3] = {LED0_PIN, LED1_PIN, LED2_PIN};
    binary_output_setup(&leds, led_pins, 3);

    value_flasher vf;
    value_flasher_setup(&vf, 954, 750);
    uint8_t old_flags = vf.out_flags+1;
    while(true){
        binary_output_mask(&leds, vf.out_flags);
        sleep_ms(10);
    }
}