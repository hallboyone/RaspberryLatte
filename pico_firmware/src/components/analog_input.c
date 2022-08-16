#include "hardware/adc.h"

#include "analog_input.h"

#include "status_ids.h"

static inline bool is_adc_pin(const uint8_t p){
    return (p >= 26) && (p <= 29);
}

/**
 * Configures an anolog input at a_pin.
 * 
 * \param a Pointer to analog_input that will be setup.
 * \param a_pin The GPIO pin for the analog input. Must be GPIO 26, 27, 28, or 29.
 * 
 * \returns 0 on success. 1 on failure (invalid GPIO is the most likely cause)
 */
int analog_input_setup(analog_input * a, uint8_t a_pin){
    if(!is_adc_pin(a_pin)) return 1;

    adc_init();
    adc_gpio_init(a_pin);
    a->pin = a_pin;
    return 0;
} 

/**
 * \brief Gets the value of the analog input.
 * 
 * \param a Previously setup analog input.
 * 
 * \returns The raw value of the analog input. 
 */
uint16_t analog_input_read(analog_input * a){
    if(is_adc_pin(a->pin)){
        adc_select_input(a->pin - 26);
        return adc_read();
    } else {
        return 0;
    }
}