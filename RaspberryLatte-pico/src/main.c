#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/cyw43_arch.h"

#include "pinout.h"

#include "maintainer.h"
#include "phasecontrol.h"
#include "binary_input.h"
#include "pressure_sensor.h"
#include "uart_bridge.h"
#include "nau7802.h"
#include "lmt01.h"
#include "heater.h"
#include "solenoid.h"
#include "leds.h"

bool run = true;

void endProgram(int * data, int len){
    run = false;
}

static bool toggle_led(repeating_timer_t *rt){
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, !cyw43_arch_gpio_get(CYW43_WL_GPIO_LED_PIN));
  return true;
}

int main(){
    // Setup wifi module NOTE: JUST LED FOR NOW. ADJUST CMAKE WHEN EXPANDED
    if (cyw43_arch_init()) return -1;
    repeating_timer_t led_timer;
    add_repeating_timer_ms(1000, &toggle_led, NULL, &led_timer);

    // Setup UART, clear queue, and assign endProgram command
    stdio_init_all();
    while(getchar_timeout_us(10) != PICO_ERROR_TIMEOUT) tight_loop_contents();
    registerHandler(MSG_ID_END_PROGRAM, &endProgram);

    pressure_sensor_setup(PRESSURE_SENSOR_PIN);

    const uint8_t pump_switch_gpio = PUMP_SWITCH_PIN;
    const uint8_t mode_select_gpio[4] = {DIAL_A_PIN, DIAL_B_PIN};
    binary_input_setup(1, &pump_switch_gpio, PULL_DOWN, false);
    binary_input_setup(2, mode_select_gpio, PULL_DOWN, true);

    // Set up phase control
    PhasecontrolConfig pump_config = 
    {.event          = FALLING,
    .zerocross_pin   = PHASE_CONTROL_0CROSS_PIN,
    .out_pin         = PHASE_CONTROL_OUT_PIN,
    .zerocross_shift = PHASE_CONTROL_0CROSS_SHIFT};
    phasecontrol_setup(&pump_config);

    nau7802_setup(SCALE_CLOCK_PIN, SCALE_DATA_PIN, i2c1);

    heater_setup(HEATER_PWM_PIN);
    
    lmt01_setup(0, LMT01_DATA_PIN);

    solenoid_setup(SOLENOID_PIN);
  
    leds_setup(LED0_PIN, LED1_PIN, LED2_PIN);

    // Continually look for a message and then run maintenance
    while(run){
        readMessage();
        runMaintenance();
    }
}