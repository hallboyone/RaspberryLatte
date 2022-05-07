#include "solenoid.h"
#include "uart_bridge.h"

static uint8_t _solenoid_pin;

/**
 * Write the target duty cycle from 0 to 127 to core1 with 0 being off. 
 */
static void solenoid_set_state_handler(int* value, int len){
  assert(len==1);
  gpio_put(_solenoid_pin, value[0]!=0);
}

/**
 * Configures a digital output and registers the message handler
 * 
 * \param pin The GPIO pin attached to SSR controling the solenoid
 */
void solenoid_setup(uint8_t pin){
    _solenoid_pin = pin;
    gpio_init(_solenoid_pin);
    gpio_set_dir(_solenoid_pin, GPIO_OUT);
    gpio_put(_solenoid_pin, 0);

    assignHandler(MSG_ID_SET_SOLENOID, &solenoid_set_state_handler);
}