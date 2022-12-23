#include <stdio.h>

#include "pinout.h"
#include "i2c_bus.h"
#include "flow_meter.h"
#include "nau7802.h"
#include "phasecontrol.h"
#include "binary_output.h"

int main(){
    // Setup UART
    stdio_uart_init_full(PICO_DEFAULT_UART_INSTANCE, 115200, PICO_DEFAULT_UART_TX_PIN, PICO_DEFAULT_UART_RX_PIN);

    flow_meter fm;
    flow_meter_setup(&fm, FLOW_RATE_PIN, 0.5);

    i2c_inst_t * bus = i2c1;
    i2c_bus_setup(bus, 100000, I2C_SCL_PIN, I2C_SDA_PIN);

    nau7802 scale;
    nau7802_setup(&scale, bus, -0.152710615479);
    discrete_derivative scale_flow;
    discrete_derivative_init(&scale_flow, 1000);

    phasecontrol pump;
    phasecontrol_setup(&pump, PHASECONTROL_0CROSS_PIN, PHASECONTROL_OUT_PIN, PHASECONTROL_0CROSS_SHIFT, ZEROCROSS_EVENT_RISING);

    uint8_t solenoid_pin [1] = {SOLENOID_PIN};
    binary_output solenoid;
    binary_output_setup(&solenoid, solenoid_pin, 1);
    binary_output_put(&solenoid, 0, 1);

    // Run main machine loop
    absolute_time_t next_msg_time = get_absolute_time();
    const uint64_t msg_period_us = 1000000;
    absolute_time_t next_loop_time;
    const uint64_t loop_period_us = 50000;
    
    uint8_t pump_vals [] = {57, 62, 67, 72, 77, 82, 87, 92, 97, 102, 107, 112, 117, 122, 127};
    for(uint i = 0; i < 15; i++){
        phasecontrol_set_duty_cycle(&pump, pump_vals[i]);
        sleep_ms(700);

        for(uint k = 0; k < 11; k++){
            datapoint scale_val = {.t = sec_since_boot(), .v = nau7802_read_mg(&scale)};
            discrete_derivative_add_point(&scale_flow, scale_val);
            sleep_ms(100);
        }
        printf("%0.2f : %0.4f\n", flow_meter_rate(&fm), discrete_derivative_read(&scale_flow)/(1000*flow_meter_rate(&fm)));

        discrete_derivative_reset(&scale_flow);
    }
    phasecontrol_set_duty_cycle(&pump, 0);
    binary_output_put(&solenoid, 0, 0);
    sleep_ms(500);
    return 0;
}