#include "hardware/adc.h"

#include "analog_input.h"

#include "status_ids.h"

/**
 * \brief Checks if pin is a valid ADC pin on Pi Pico (26-29)
 * 
 * \param p GPIO number to check
 * \returns true if valid ADC pin. Else, returns false.
 */
static inline bool is_adc_pin(const uint8_t p){
    return (p >= 26) && (p <= 29);
}

int analog_input_setup(analog_input * a, uint8_t a_pin){
    if(!is_adc_pin(a_pin)) return 1;

    adc_init();
    adc_gpio_init(a_pin);
    a->pin = a_pin;
    return 0;
} 

uint16_t analog_input_read(analog_input * a){
    if(is_adc_pin(a->pin)){
        adc_select_input(a->pin - 26);
        return adc_read();
    } else {
        return 0;
    }
}