#include "hardware/adc.h"

#include "pressure_sensor.h"
#include "uart_bridge.h"

static uint8_t _a_pin = 255;

/**
 * \brief Gets the value of the pressure sensor attached to configured pin.
 * 
 * \returns The raw value of the configured pressure sensor. 
 */
int pressure_sensor_read(){
  if(_a_pin != 255){
    adc_select_input(_a_pin - 26);
  }
  return adc_read();
}

/**
 * Responds to requests for the current pressure by packing the int uint16_t into bytes and sending response.
 */
static void pressure_sensor_read_handler(int* data, int len){
  int response [2] = {0,0};
  if(_a_pin != 255){
    adc_select_input(_a_pin - 26);
    int pressure = adc_read();
    response[0] = (pressure>>8) & 0xFF;
    response[1] = (pressure>>0) & 0xFF;
  }
  sendMessage(MSG_ID_READ_PRESSURE, response, 2);
}

/**
 * Configures an anolog input at a_pin to read a pressure sensor and registers as a message handler.
 * 
 * \param a_pin The GPIO pin that the pressure signal is attached to. Must be GPIO 26, 27, 28, or 29.
 */
void pressure_sensor_setup(uint8_t a_pin){
  adc_init();
  adc_gpio_init(a_pin);

  _a_pin = a_pin;

  assignHandler(MSG_ID_READ_PRESSURE, &pressure_sensor_read_handler);
} 