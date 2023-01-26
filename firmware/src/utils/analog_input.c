/**
 * \ingroup analog_input
 * 
 * \file
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Analog Input Library source
 * \version 0.1
 * \date 2022-08-16
 */

#include "utils/analog_input.h"

#include "hardware/adc.h"

/**
 * \brief Checks if pin is a valid ADC pin on Pi Pico (26-29)
 * 
 * \param p GPIO number to check
 * \returns true if valid ADC pin. Else, returns false.
 */
static inline bool _is_adc_pin(const uint8_t p){
    return (p >= 26) && (p <= 29);
}

int analog_input_setup(analog_input * a, uint8_t a_pin, float conversion_factor){
    if(!_is_adc_pin(a_pin)) return PICO_ERROR_INVALID_ARG;

    adc_init();
    adc_gpio_init(a_pin);
    a->pin = a_pin;
    a->conversion_factor = conversion_factor;
    return PICO_ERROR_NONE;
} 

float analog_input_read(analog_input * a){
    return a->conversion_factor * analog_input_read_raw(a);
}

uint16_t analog_input_read_raw(analog_input * a){
    if(_is_adc_pin(a->pin)){
        adc_select_input(a->pin - 26);
        return adc_read();
    } else {
        return 0;
    }
}