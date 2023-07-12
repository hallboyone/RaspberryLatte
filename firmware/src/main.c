#define PRINT_MACHINE_STATUS_OVER_UART

#ifdef PRINT_MACHINE_STATUS_OVER_UART
#include <stdio.h>
#endif

#include "config/raspberry_latte_config.h"
#include "config/pinout.h"

#include "utils/i2c_bus.h"

#include "drivers/ulka_pump.h"
#include "drivers/flow_meter.h"
#include "utils/binary_input.h"
#include "drivers/nau7802.h"

//#include "machine_logic/espresso_machine.h"
/** I2C bus connected to memory, scale ADC, and I2C port */
static i2c_inst_t *  bus = i2c1;
static binary_input  pump_switch; /**< Monitors the state of the pump switch. */
static binary_input  mode_dial;   /**< Monitors the state of the mode dial. */
static nau7802       scale;       /**< Output scale. */
static ulka_pump     pump;        /**< The vibratory pump. */
static flow_meter    fm;

int main(){
    // Setup UART
    stdio_uart_init_full(PICO_DEFAULT_UART_INSTANCE, 115200, PICO_DEFAULT_UART_TX_PIN, PICO_DEFAULT_UART_RX_PIN);

    //espresso_machine_viewer espresso_machine;
    //if(espresso_machine_setup(&espresso_machine)) return 1;

    i2c_bus_setup(bus, 100000, I2C_SCL_PIN, I2C_SDA_PIN);

    // Setup the binary inputs for pump switch and mode dial.
    const uint8_t pump_switch_gpio = PUMP_SWITCH_PIN;
    pump_switch = binary_input_setup(1, &pump_switch_gpio, BINARY_INPUT_PULL_UP, PUMP_SWITCH_DEBOUNCE_DURATION_US, false, false);
    bool switch_on = binary_input_read(pump_switch);
    // Setup the pump
    pump = ulka_pump_setup(AC_0CROSS_PIN, PUMP_OUT_PIN, AC_0CROSS_SHIFT, ZEROCROSS_EVENT_RISING);
    
    // Setup nau7802
    scale = nau7802_setup(bus, SCALE_CONVERSION_MG);
    
    fm = flow_meter_setup(FLOW_RATE_PIN, 1., 50, 10);

    for(uint8_t p = 5; p <= 100; p += 5){
        while(switch_on == binary_input_read(pump_switch)){
            sleep_ms(25);
        }
        ulka_pump_pwr_percent(pump, p);
        sleep_ms(2000);
        flow_meter_zero(fm);
        nau7802_zero(scale);
        sleep_ms(10000);
        const int output = nau7802_read_mg(scale);
        const float pulses = flow_meter_volume(fm);
        const float conversion = output/pulses;
        ulka_pump_pwr_percent(pump, 0);
        printf("%d\t%0.6f", p, conversion);
        switch_on = binary_input_read(pump_switch);
    }

/*
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
                printf("%5.1f\t%5.1f\t%3d\t%2d\t%3d\t%6.1f\t%4.1f\n",
                espresso_machine->boiler.setpoint/100.,
                espresso_machine->boiler.temperature/100.,
                espresso_machine->boiler.power_level,
                espresso_machine->autobrew_leg,
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
    */
}