#include <stdio.h>

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


//         if(last_msg_time + 1000000 <= time_us_64()){
//             last_msg_time += 1000000;
//             printf("T = %0.2f / %0.2f\n", espresso_machine->boiler.temperature/16., espresso_machine->boiler.setpoint/16.);
//         }
//         loop_rate_limiter_us(&last_loop_time_us, 10000);
//     }
// }

#include "mb85_fram.h"
#include "pinout.h"

typedef struct {
    int a;
    uint8_t b;
    float c;
} mystruct_t;

int main(){
    stdio_uart_init_full(PICO_DEFAULT_UART_INSTANCE, 115200, PICO_DEFAULT_UART_TX_PIN, PICO_DEFAULT_UART_RX_PIN);
    sleep_ms(100);

    i2c_inst_t * bus = i2c1;
    i2c_bus_setup(bus, 100000, I2C_SCL_PIN, I2C_SDA_PIN);

    mb85_fram fram;
    mb85_fram_setup(&fram, bus, 0b000, NULL);

    const reg_addr MEMORY_LOCATION_0 = 0;

    float var_1 = 3.1415;
    float var_2 = 0;
    mb85_fram_link_var(&fram, &var_1, MEMORY_LOCATION_0, sizeof(float), MB85_FRAM_INIT_FROM_VAR);
    mb85_fram_link_var(&fram, &var_2, MEMORY_LOCATION_0, sizeof(float), MB85_FRAM_INIT_FROM_FRAM);

    if(var_2 != var_1){
        printf("\nFailed test 1\n");
        return PICO_ERROR_GENERIC;
    } else {
        printf("\nPassed test 1\n");
    }

    var_2 += 1;
    mb85_fram_save(&fram, &var_2);
    mb85_fram_load(&fram, &var_1);
    if(var_2 != var_1){
        printf("Failed test 2\n");
        return PICO_ERROR_GENERIC;
    } else {
        printf("Passed test 2\n");
    }

    const reg_addr MEMORY_LOCATION_1 = 64;

    mystruct_t s1 = {.a = -10, .b = 16, .c = 3.1415};
    mystruct_t s2 = {.a = 15, .b = 3, .c = 6.1415};
    mb85_fram_link_var(&fram, &s1, MEMORY_LOCATION_1, sizeof(mystruct_t), MB85_FRAM_INIT_FROM_VAR);
    mb85_fram_link_var(&fram, &s2, MEMORY_LOCATION_1, sizeof(mystruct_t), MB85_FRAM_INIT_FROM_FRAM);
    if(s1.c != s2.c){
        printf("Failed test 3\n");
        return PICO_ERROR_GENERIC;
    } else {
        printf("Passed test 3\n");
    }

    if(mb85_fram_get_max_addr(&fram) != 8191){
        printf("Failed test 4\n");
        return PICO_ERROR_GENERIC;
    } else {
        printf("Passed test 4\n");
    }

    mb85_fram_load(&fram, &s1);
    if(s1.c != s2.c){
        printf("Failed test 5\n");
        return PICO_ERROR_GENERIC;
    } else {
        printf("Passed test 5\n");
    }

    mb85_fram_set_all(&fram, 0xAA);
    uint8_t single_byte;    
    mb85_fram_link_var(&fram, &single_byte, 128, 1, MB85_FRAM_INIT_FROM_FRAM);
    if(single_byte != 0xAA){
        printf("Failed test 6\n");
        return PICO_ERROR_GENERIC;
    } else {
        printf("Passed test 6\n");
    }

    uint16_t var_3 = 1234;
    uint16_t var_4 = 4321;
    mb85_fram_link_var(&fram, &var_3, 365, 2, MB85_FRAM_INIT_FROM_VAR);
    mb85_fram_link_var(&fram, &var_4, 367, 2, MB85_FRAM_INIT_FROM_VAR);
    mb85_fram_unlink_var(&fram, &var_3);
    mb85_fram_link_var(&fram, &var_3, 367, 2, MB85_FRAM_INIT_FROM_FRAM);
    if(var_3 != 4321){
        printf("Failed test 7\n");
        return PICO_ERROR_GENERIC;
    } else {
        printf("Passed test 7\n");
    }
    
    return PICO_ERROR_NONE;
}