#include "nau7802.h"

const dev_addr _nau7802_addr = 0x2A;     /**< I2C address of the NAU7802 IC */

const reg_addr REG_PU_CTRL = 0x0;  /**< NAU7802 register address: Power-up control */
const reg_addr REG_CTRL_1  = 0x01; /**< NAU7802 register address: Configuration 1 */
const reg_addr REG_CTRL_2  = 0x02; /**< NAU7802 register address: Configuration 2 */
const reg_addr REG_I2C_CTRL= 0x11; /**< NAU7802 register address: I2C Configuation*/
const reg_addr REG_ADCO_B2 = 0x12; /**< NAU7802 register address: Conversion result 23-16*/
const reg_addr REG_ADCO_B1 = 0x13; /**< NAU7802 register address: Conversion result 15-8*/
const reg_addr REG_ADCO_B0 = 0x14; /**< NAU7802 register address: Conversion result 7-0*/
const reg_addr REG_ADC_CTRL= 0x15; /**< NAU7802 register address: ADC configuration */
const reg_addr REG_PGA     = 0x1B; /**< NAU7802 register address: Programmable gain amp config */
const reg_addr REG_PWR_CTRL= 0x1C; /**< NAU7802 register address: Power control */
const reg_addr REG_DEV_REV = 0x1F; /**< NAU7802 register address: Device information */

const bit_range BITS_RESET       = {.from = 0, .to = 0, .in_reg = REG_PU_CTRL, .reg_addr_len = 1};
const bit_range BITS_PWR_UP_D    = {.from = 1, .to = 1, .in_reg = REG_PU_CTRL, .reg_addr_len = 1};
const bit_range BITS_PWR_UP_A    = {.from = 2, .to = 2, .in_reg = REG_PU_CTRL, .reg_addr_len = 1};
const bit_range BITS_READY       = {.from = 3, .to = 3, .in_reg = REG_PU_CTRL, .reg_addr_len = 1};
const bit_range BITS_CS          = {.from = 4, .to = 4, .in_reg = REG_PU_CTRL, .reg_addr_len = 1};
const bit_range BITS_CR          = {.from = 5, .to = 5, .in_reg = REG_PU_CTRL, .reg_addr_len = 1};
const bit_range BITS_OSCS        = {.from = 6, .to = 6, .in_reg = REG_PU_CTRL, .reg_addr_len = 1};
const bit_range BITS_AVDD_S      = {.from = 7, .to = 7, .in_reg = REG_PU_CTRL, .reg_addr_len = 1};

const bit_range BITS_GAIN        = {.from = 0, .to = 2, .in_reg = REG_CTRL_1, .reg_addr_len = 1};
const bit_range BITS_VLDO        = {.from = 3, .to = 5, .in_reg = REG_CTRL_1, .reg_addr_len = 1};
const bit_range BITS_DRDY_SEL    = {.from = 6, .to = 6, .in_reg = REG_CTRL_1, .reg_addr_len = 1};
const bit_range BITS_CRP         = {.from = 7, .to = 7, .in_reg = REG_CTRL_1, .reg_addr_len = 1};

const bit_range BITS_CAL_MODE    = {.from = 0, .to = 1, .in_reg = REG_CTRL_2, .reg_addr_len = 1};
const bit_range BITS_CALS        = {.from = 2, .to = 2, .in_reg = REG_CTRL_2, .reg_addr_len = 1};
const bit_range BITS_CAL_ERR     = {.from = 3, .to = 3, .in_reg = REG_CTRL_2, .reg_addr_len = 1};
const bit_range BITS_CRS         = {.from = 4, .to = 6, .in_reg = REG_CTRL_2, .reg_addr_len = 1};
const bit_range BITS_CHS         = {.from = 7, .to = 7, .in_reg = REG_CTRL_2, .reg_addr_len = 1};

const bit_range BITS_BGPCP       = {.from = 0, .to = 0, .in_reg = REG_I2C_CTRL, .reg_addr_len = 1};
const bit_range BITS_TS          = {.from = 1, .to = 1, .in_reg = REG_I2C_CTRL, .reg_addr_len = 1};
const bit_range BITS_BOPGA       = {.from = 2, .to = 2, .in_reg = REG_I2C_CTRL, .reg_addr_len = 1};
const bit_range BITS_SI          = {.from = 3, .to = 3, .in_reg = REG_I2C_CTRL, .reg_addr_len = 1};
const bit_range BITS_WPD         = {.from = 4, .to = 4, .in_reg = REG_I2C_CTRL, .reg_addr_len = 1};
const bit_range BITS_SPE         = {.from = 5, .to = 5, .in_reg = REG_I2C_CTRL, .reg_addr_len = 1};
const bit_range BITS_FRD         = {.from = 6, .to = 6, .in_reg = REG_I2C_CTRL, .reg_addr_len = 1};
const bit_range BITS_CRSD        = {.from = 7, .to = 7, .in_reg = REG_I2C_CTRL, .reg_addr_len = 1};

const bit_range BITS_B23_16      = {.from = 0, .to = 7, .in_reg = REG_ADCO_B2, .reg_addr_len = 1};

const bit_range BITS_B15_08      = {.from = 0, .to = 7, .in_reg = REG_ADCO_B1, .reg_addr_len = 1};

const bit_range BITS_B07_00      = {.from = 0, .to = 7, .in_reg = REG_ADCO_B0, .reg_addr_len = 1};

const bit_range BITS_REG_CHP     = {.from = 0, .to = 1, .in_reg = REG_ADC_CTRL, .reg_addr_len = 1};
const bit_range BITS_ADC_VCM     = {.from = 2, .to = 3, .in_reg = REG_ADC_CTRL, .reg_addr_len = 1};
const bit_range BITS_REG_CHPS    = {.from = 4, .to = 5, .in_reg = REG_ADC_CTRL, .reg_addr_len = 1};

const bit_range BITS_PGA_CHP_DIS = {.from = 0, .to = 0, .in_reg = REG_PGA, .reg_addr_len = 1};
const bit_range BITS_PGA_INV     = {.from = 3, .to = 3, .in_reg = REG_PGA, .reg_addr_len = 1};
const bit_range BITS_PGA_BYP_EN  = {.from = 4, .to = 4, .in_reg = REG_PGA, .reg_addr_len = 1};
const bit_range BITS_PGA_OBUF_EN = {.from = 5, .to = 5, .in_reg = REG_PGA, .reg_addr_len = 1};
const bit_range BITS_LDO_MODE    = {.from = 6, .to = 6, .in_reg = REG_PGA, .reg_addr_len = 1};
const bit_range BITS_RD_OTP_SEL  = {.from = 7, .to = 7, .in_reg = REG_PGA, .reg_addr_len = 1};

const bit_range BITS_PGA_CURR    = {.from = 0, .to = 1, .in_reg = REG_PWR_CTRL, .reg_addr_len = 1};
const bit_range BITS_ADC_CURR    = {.from = 2, .to = 3, .in_reg = REG_PWR_CTRL, .reg_addr_len = 1};
const bit_range BITS_MST_BS_CURR = {.from = 4, .to = 6, .in_reg = REG_PWR_CTRL, .reg_addr_len = 1};
const bit_range BITS_PGA_CAP     = {.from = 7, .to = 7, .in_reg = REG_PWR_CTRL, .reg_addr_len = 1};

const bit_range BITS_REVISION_ID = {.from = 0, .to = 3, .in_reg = REG_DEV_REV, .reg_addr_len = 1};

int nau7802_read_reg(nau7802 * scale, const reg_addr reg_idx, uint8_t len, uint8_t * dst){
    return i2c_bus_read_bytes(scale->bus, _nau7802_addr, reg_idx, 1, len, dst);
}

int nau7802_write_reg(nau7802 * scale, const reg_addr reg_idx, uint8_t len, uint8_t * src){
    return i2c_bus_write_bytes(scale->bus, _nau7802_addr, reg_idx, 1, len, src);
}

int nau7802_read_bits(nau7802 * scale, const bit_range bits, uint8_t * dst){
    return i2c_bus_read_bits(scale->bus, _nau7802_addr, bits, dst);
}

int nau7802_write_bits(nau7802 * scale, const bit_range bits, uint8_t val){
    return i2c_bus_write_bits(scale->bus, _nau7802_addr, bits, val);
}

int nau7802_reset(nau7802 * scale){
    int result = nau7802_write_bits(scale, BITS_RESET, 1);
    if(result != I2C_BUS_SUCCESS){
        return result;
    }
    sleep_ms(1);
    result = nau7802_write_bits(scale, BITS_RESET, 0);
    return result;
}

bool nau7802_is_ready(nau7802 * scale){
    uint8_t ready_bit = 0;
    if(nau7802_read_bits(scale, BITS_READY, &ready_bit) != I2C_BUS_SUCCESS){
        return false;
    }
    return ready_bit;
}

bool nau7802_wait_till_ready_ms(nau7802 * scale, uint timeout){
    uint64_t end_time = 1000*timeout + time_us_64();
    while(!nau7802_is_ready(scale)){
        if (time_us_64() > end_time) return false;
    }
    return true;
}

int nau7802_set_analog_power_supply(nau7802 * scale, avdd_src source){
    return nau7802_write_bits(scale, BITS_AVDD_S, source);
}

int nau7802_set_digital_power(nau7802 * scale, pwr_setting on_off){
    return nau7802_write_bits(scale, BITS_PWR_UP_D, on_off);
}

int nau7802_set_analog_power(nau7802 * scale, pwr_setting on_off){
    return nau7802_write_bits(scale, BITS_PWR_UP_A, on_off);
}

int nau7802_set_conversions(nau7802 * scale, conversion_setting on_off){
    return nau7802_write_bits(scale, BITS_CS, on_off);
}

int nau7802_set_gain(nau7802 * scale, gain g){
    return nau7802_write_bits(scale, BITS_GAIN, g);
}

int nau7802_set_ldo_voltage(nau7802 * scale, ldo_voltage v){
    return nau7802_write_bits(scale, BITS_VLDO, v);
}

int nau7802_set_ldo_mode(nau7802 * scale, ldo_mode mode){
    return nau7802_write_bits(scale, BITS_LDO_MODE, mode);
}

int nau7802_set_chopper_clock(nau7802 * scale, chp_clk val){
    return nau7802_write_bits(scale, BITS_REG_CHPS, val);
}

int nau7802_set_pga_filter(nau7802 * scale, pga_setting off_on){
    return nau7802_write_bits(scale, BITS_PGA_CAP, off_on);
}

int nau7802_calibrate(nau7802 * scale){
    nau7802_write_bits(scale, BITS_CALS, 1);

    uint64_t end_time = 1000000 + time_us_64();
    uint8_t buf = 1;
    while(buf){
        sleep_ms(1);
        if (time_us_64() > end_time) return false;
        nau7802_read_bits(scale, BITS_CALS, &buf);
    }

    nau7802_read_bits(scale, BITS_CAL_ERR, &buf);
    if(buf) return -1;
    else return 0;
}

bool nau7802_data_ready(nau7802 * scale){
    uint8_t is_ready = 0;
    int result = nau7802_read_bits(scale, BITS_CR, &is_ready);
    if(result != I2C_BUS_SUCCESS) return false;

    return is_ready;
}

int nau7802_read_raw(nau7802 * scale, uint32_t * dst){
    if (nau7802_data_ready(scale)){
        int result = nau7802_read_reg(scale, REG_ADCO_B2, 3, (uint8_t*)dst);
        if(result != I2C_BUS_SUCCESS) return result;

        uint32_t b0 = ((*dst)&0xFF0000);
        uint32_t b1 = ((*dst)&0x00FF00);
        uint32_t b2 = ((*dst)&0x0000FF);
        *dst = (b0>>16) | (b1) | (b2<<16);
        scale->latest_val = *dst;
    } else {
        *dst = scale->latest_val;
    }
    return I2C_BUS_SUCCESS;
}

int nau7802_read_mg(nau7802 * scale){
    uint32_t val;
    if(nau7802_read_raw(scale, &val)!= I2C_BUS_SUCCESS){
        return 0;
    }

    return scale->conversion_factor_mg*((int)val-(int)scale->origin);
}

int nau7802_zero(nau7802 * scale){
    if(nau7802_read_raw(scale, &scale->origin)!= I2C_BUS_SUCCESS){
        return PICO_ERROR_GENERIC;
    }
}

bool nau7802_at_val_mg(nau7802 * scale, int val){
    return nau7802_read_mg(scale) >= val;
}

/**
 * \brief Assign standard values to the registers of the NAU7802 chip.
 * 
 * \returns PICO_OK if setup successfully. Else returns error code.
 */
static int _nau7802_setup(nau7802 * scale){
    if(!i2c_bus_is_connected(scale->bus, _nau7802_addr)){
        return PICO_ERROR_GENERIC;
    }
    if(nau7802_reset(scale)){
        return PICO_ERROR_GENERIC;
    }
    if(nau7802_set_digital_power(scale, PWR_ON)){
        return PICO_ERROR_GENERIC;
    }
    if(nau7802_set_analog_power_supply(scale, AVDD_SRC_INTERNAL)){
        return PICO_ERROR_GENERIC;
    }
    if(!nau7802_wait_till_ready_ms(scale, 25)){
        return PICO_ERROR_TIMEOUT;
    }
    if(nau7802_set_analog_power(scale, PWR_ON)){
        return PICO_ERROR_GENERIC;
    }
    if(nau7802_set_ldo_voltage(scale, VLDO_3_0)){
        return PICO_ERROR_GENERIC;
    }
    if(nau7802_set_ldo_mode(scale, LDO_MODE_ACCURATE)){
        return PICO_ERROR_GENERIC;
    }
    if(nau7802_set_gain(scale, GAIN_128)){
        return PICO_ERROR_GENERIC;
    }
    if(nau7802_set_chopper_clock(scale, CHP_CLK_OFF)){
        return PICO_ERROR_GENERIC;
    }
    if(nau7802_set_pga_filter(scale, PGA_ON)){
        return PICO_ERROR_GENERIC;
    }
    if(nau7802_calibrate(scale)){
        return PICO_ERROR_GENERIC;
    }
    if(nau7802_set_conversions(scale, CONVERSIONS_ON)){
        return PICO_ERROR_GENERIC;
    }

    return PICO_OK;
}

int nau7802_setup(nau7802 * scale, i2c_inst_t * nau7802_i2c, float conversion_factor_mg){
    scale->bus = nau7802_i2c;
    scale->conversion_factor_mg = conversion_factor_mg;
    scale->latest_val = 0;
    scale->origin = 0;

    // Try to setup scale up to ten times.
    for(int i = 0; i<10; i++){
        if(_nau7802_setup(scale) == PICO_ERROR_NONE) break;
        if(i==9){
            return PICO_ERROR_GENERIC;
        } 
    }

    while(scale->origin == 0){
        nau7802_zero(scale);
    }
}