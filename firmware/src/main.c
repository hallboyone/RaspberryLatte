#include <stdio.h>

#include "machine_logic/espresso_machine.h"
#include "utils/pid.h"
#include "machine_logic/machine_settings.h"
#include "pinout.h"

#define S_TO_US 1000000
#define MS_TO_US 1000

const float data [] = {
    0.00,0.96,1.90,2.76,3.64,4.44,5.15,5.89,6.59,7.09,
    7.58,7.98,8.42,8.74,9.06,9.28,9.41,9.64,9.82,9.78,
    9.80,9.80,9.77,9.57,9.45,9.11,8.76,8.47,8.07,7.67,
    7.24,6.69,6.06,5.40,4.73,3.99,3.28,2.36,1.44,0.39};

int main(){
    // Setup UART
    stdio_uart_init_full(PICO_DEFAULT_UART_INSTANCE, 115200, PICO_DEFAULT_UART_TX_PIN, PICO_DEFAULT_UART_RX_PIN);

    int64_t time_sum_1 = 0;
    int64_t time_sum_2 = 0;

    discrete_derivative d = discrete_derivative_setup(4, 1);
    absolute_time_t next_loop_time;
    for(int i = 0; i < 40; i++){
        const datapoint dp = {.v=data[i], .t=i};
        absolute_time_t t = get_absolute_time();
        discrete_derivative_add_datapoint(d, dp);
        time_sum_1 += absolute_time_diff_us(t, get_absolute_time());

        if(i%5==4) discrete_derivative_print(d);
    }
    printf("%0.4f\n", (float)time_sum_1/1000.0);
    // espresso_machine_viewer espresso_machine;
    // if(espresso_machine_setup(&espresso_machine)){
    //     return 1;
    // }
    // machine_settings * settings = machine_settings_acquire();

    // // Message rate limiter
    // absolute_time_t next_msg_time = get_absolute_time();
    // const uint64_t msg_period_us = 200 * MS_TO_US;

    // // Loop rate limiter
    // absolute_time_t next_loop_time;
    // const uint64_t loop_period_us = 10 * MS_TO_US;

    // while(true){
    //     next_loop_time = make_timeout_time_us(loop_period_us);

    //     // Update the machine
    //     espresso_machine_tick();
    //     if(espresso_machine->pump.flowrate_ml_s < 0){
    //         printf("Warning\n");
    //     }
    //     // Print status once a second
    //     if(absolute_time_diff_us(get_absolute_time(), next_msg_time) < 0){
    //         next_msg_time = make_timeout_time_us(msg_period_us);
    //         if(espresso_machine->switches.ac_switch){
    //             const uint32_t timestamp_ms = to_ms_since_boot(get_absolute_time());
    //             printf("%07d,%0.2f,%0.2f,%03d,%0.3f\n",
    //             timestamp_ms,
    //             espresso_machine->boiler.setpoint/16.,
    //             espresso_machine->boiler.temperature/16.,
    //             espresso_machine->pump.power_level,
    //             espresso_machine->pump.flowrate_ml_s);
    //         }
    //     }
    //     sleep_until(next_loop_time);
    // }
}