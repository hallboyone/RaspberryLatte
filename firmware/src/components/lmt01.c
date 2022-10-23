#include "hardware/clocks.h"

#include "lmt01.h"
#include "lmt01.pio.h"

#include "status_ids.h"

static const int PULSE_COUNTS [21] = { 26, 181, 338, 494, 651, 808,
                                       966, 1125, 1284, 1443, 1603, 
                                      1762, 1923, 2084, 2245, 2407, 
                                      2569, 2731, 2894, 3058, 3220};

static const float PULSE_SLOPES [20] = {1.03226,1.01911,1.02564,1.01911,1.01911,
                                        1.01266,1.00629,1.00629,1.00629,1.00629,
                                        1.00000,0.99379,0.99379,0.99379,0.98765,
                                        0.98765,0.98765,0.98765,0.97561,0.99379};

static const int PULSE_SHIFTS [20] = {-827, -824, -827, -823, -823, 
                                      -818, -812, -812, -812, -812,
                                      -802, -791, -791, -791, -777, 
                                      -777, -777, -777, -742, -798};

/**
 * \brief Converts a pulse count to the corresponding temperature multiplied by 16
 * 
 * \param pulse_count the number of pulses from the LMT01 sensor
 * \return The corresponding temp. Divide by 16 to get temp in C
 */
static inline int pulse2Temp(const int pulse_count){
    if (pulse_count < PULSE_COUNTS[1]){
        return (pulse_count*PULSE_SLOPES[0] + PULSE_SHIFTS[0]);
    } else if (pulse_count < PULSE_COUNTS[2]){
        return (pulse_count*PULSE_SLOPES[1] + PULSE_SHIFTS[1]);
    } else if (pulse_count < PULSE_COUNTS[3]){
        return (pulse_count*PULSE_SLOPES[2] + PULSE_SHIFTS[2]);
    } else if (pulse_count < PULSE_COUNTS[4]){
        return (pulse_count*PULSE_SLOPES[3] + PULSE_SHIFTS[3]);
    } else if (pulse_count < PULSE_COUNTS[5]){
        return (pulse_count*PULSE_SLOPES[4] + PULSE_SHIFTS[4]);
    } else if (pulse_count < PULSE_COUNTS[6]){
        return (pulse_count*PULSE_SLOPES[5] + PULSE_SHIFTS[5]);
    } else if (pulse_count < PULSE_COUNTS[7]){
        return (pulse_count*PULSE_SLOPES[6] + PULSE_SHIFTS[6]);
    } else if (pulse_count < PULSE_COUNTS[8]){
        return (pulse_count*PULSE_SLOPES[7] + PULSE_SHIFTS[7]);
    } else if (pulse_count < PULSE_COUNTS[9]){
        return (pulse_count*PULSE_SLOPES[8] + PULSE_SHIFTS[8]);
    } else if (pulse_count < PULSE_COUNTS[10]){
        return (pulse_count*PULSE_SLOPES[9] + PULSE_SHIFTS[9]);
    } else if (pulse_count < PULSE_COUNTS[11]){
        return (pulse_count*PULSE_SLOPES[10] + PULSE_SHIFTS[10]);
    } else if (pulse_count < PULSE_COUNTS[12]){
        return (pulse_count*PULSE_SLOPES[11] + PULSE_SHIFTS[11]);
    } else if (pulse_count < PULSE_COUNTS[13]){
        return (pulse_count*PULSE_SLOPES[12] + PULSE_SHIFTS[12]);
    } else if (pulse_count < PULSE_COUNTS[14]){
        return (pulse_count*PULSE_SLOPES[13] + PULSE_SHIFTS[13]);
    } else if (pulse_count < PULSE_COUNTS[15]){
        return (pulse_count*PULSE_SLOPES[14] + PULSE_SHIFTS[14]);
    } else if (pulse_count < PULSE_COUNTS[16]){
        return (pulse_count*PULSE_SLOPES[15] + PULSE_SHIFTS[15]);
    } else if (pulse_count < PULSE_COUNTS[17]){
        return (pulse_count*PULSE_SLOPES[16] + PULSE_SHIFTS[16]);
    } else if (pulse_count < PULSE_COUNTS[18]){
        return (pulse_count*PULSE_SLOPES[17] + PULSE_SHIFTS[17]);
    } else if (pulse_count < PULSE_COUNTS[19]){
        return (pulse_count*PULSE_SLOPES[18] + PULSE_SHIFTS[18]);
    } else {
        return (pulse_count*PULSE_SLOPES[19] + PULSE_SHIFTS[19]);
    }
}

static inline void lmt01_program_init(lmt01 * l, uint offset) {
    // Setup dat_pin
    pio_sm_set_consecutive_pindirs(l->_pio, l->_sm, l->_dat_pin, 1, false);
    pio_gpio_init(l->_pio, l->_dat_pin);

    pio_sm_config c = lmt01_program_get_default_config(offset);
    
    sm_config_set_jmp_pin(&c, l->_dat_pin);
    sm_config_set_in_pins(&c, l->_dat_pin);
    
    sm_config_set_in_shift(&c, false, true, 32);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
    
    // Each clock cycle should be 0.5us -> 2_000_000 Hz
    float div = (float)clock_get_hz(clk_sys)/2000000.0;
    sm_config_set_clkdiv(&c, div);
    
    pio_sm_init(l->_pio, l->_sm, offset, &c);
    pio_sm_set_enabled(l->_pio, l->_sm, true);
}

int lmt01_read(lmt01 * l){
    while(!pio_sm_is_rx_fifo_empty(l->_pio, l->_sm)){
        l->_latest_temp = pulse2Temp(pio_sm_get_blocking(l->_pio, l->_sm));
    }
    return l->_latest_temp;
}

float lmt01_read_float(lmt01 * l){
    return lmt01_read(l)/16.;
}

void lmt01_setup(lmt01 * l, uint8_t pio_num, uint8_t dat_pin){
    l->_dat_pin = dat_pin;

    // Load pio program into memory
    l->_pio =  (pio_num==0 ? pio0 : pio1);
    uint offset = pio_add_program(l->_pio, &lmt01_program);
    l->_sm = pio_claim_unused_sm(l->_pio, true);
    lmt01_program_init(l, offset);

    while(l->_latest_temp<=0 || l->_latest_temp>2800){
        // Wait till a valid temperature is measured
        lmt01_read(l);
    }
}
