#include "nau7802.h"

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

static uint32_t last_val = 0;            /**< Last ADC reading */
static uint32_t _scale_origin = 0;       /**< Origin of the scale */
static float _conversion_factor_mg = 1;  /**< Value that converts raw ADC reading to mg */

i2c_inst_t * _nau7802_i2c = i2c_default; /**< I2C Channel to use for the NAU7802.*/
const uint16_t _nau7802_addr = 0x2A;     /**< I2C address of the NAU7802 IC */

int nau7802_read_reg(const reg_addr reg_idx, uint8_t len, uint8_t * dst){
    return i2c_bus_read_bytes(_nau7802_i2c, _nau7802_addr, reg_idx, 1, len, dst);
}

int nau7802_write_reg(const reg_addr reg_idx, uint8_t len, uint8_t * src){
    return i2c_bus_write_bytes(_nau7802_i2c, _nau7802_addr, reg_idx, 1, len, src);
}

int nau7802_read_bits(const bit_range bits, uint8_t * dst){
    return i2c_bus_read_bits(_nau7802_i2c, _nau7802_addr, bits, dst);
}

int nau7802_write_bits(const bit_range bits, uint8_t val){
    return i2c_bus_write_bits(_nau7802_i2c, _nau7802_addr, bits, val);
}

int nau7802_reset(){
    int result = nau7802_write_bits(BITS_RESET, 1);
    if(result != I2C_SUCCESS){
        return result;
    }
    sleep_ms(1);
    result = nau7802_write_bits(BITS_RESET, 1);
    return result;
}

bool nau7802_is_ready(){
    uint8_t ready_bit = 0;
    if(nau7802_read_bits(BITS_READY, &ready_bit) != I2C_SUCCESS){
        return false;
    }
    return ready_bit;
}

bool nau7802_wait_till_ready_ms(uint timeout){
    uint64_t end_time = 1000*timeout + time_us_64();
    while(!nau7802_is_ready()){
        if (time_us_64() > end_time) return false;
    }
    return true;
}

int nau7802_set_analog_power_supply(avdd_src source){
    return nau7802_write_bits(BITS_AVDD_S, source);
}

int nau7802_set_digital_power(pwr_setting on_off){
    return nau7802_write_bits(BITS_PWR_UP_D, on_off);
}

int nau7802_set_analog_power(pwr_setting on_off){
    return nau7802_write_bits(BITS_PWR_UP_A, on_off);
}

int nau7802_set_conversions(conversion_setting on_off){
    return nau7802_write_bits(BITS_CS, on_off);
}

int nau7802_set_gain(gain g){
    return nau7802_write_bits(BITS_GAIN, g);
}

int nau7802_set_ldo_voltage(ldo_voltage v){
    return nau7802_write_bits(BITS_VLDO, v);
}

int nau7802_set_ldo_mode(ldo_mode mode){
    return nau7802_write_bits(BITS_LDO_MODE, mode);
}

int nau7802_set_chopper_clock(chp_clk val){
    return nau7802_write_bits(BITS_REG_CHPS, val);
}

int nau7802_set_pga_filter(pga_setting off_on){
    return nau7802_write_bits(BITS_PGA_CAP, off_on);
}

bool nau7802_data_ready(){
    uint8_t is_ready = 0;
    int result = nau7802_read_bits(BITS_CR, &is_ready);
    if(result != I2C_SUCCESS) return false;

    return is_ready;
}

int nau7802_read_raw(uint32_t * dst){
    if (nau7802_data_ready()){
        int result = nau7802_read_reg(REG_ADCO_B2, 3, (uint8_t*)dst);
        if(result != I2C_SUCCESS) return result;

        uint32_t b0 = ((*dst)&0xFF0000);
        uint32_t b1 = ((*dst)&0x00FF00);
        uint32_t b2 = ((*dst)&0x0000FF);
        *dst = (b0>>16) | (b1) | (b2<<16);
        last_val = *dst;
    } else {
        *dst = last_val;
    }
}

int nau7802_read_mg(){
    uint32_t val;
    if(nau7802_read_raw(&val)!= I2C_SUCCESS){
        return 0;
    }

    return _conversion_factor_mg*((int)val-(int)_scale_origin);
}

int nau7802_zero(){
    if(nau7802_read_raw(&_scale_origin)!= I2C_SUCCESS){
        return PICO_ERROR_GENERIC;
    }
}

bool nau7802_at_val_mg(int val){
    return nau7802_read_mg() >= val;
}

/**
 * \brief Prepare the GPIO pins and I2C bank to interface with the NAU7802. The GPIO functions are
 * set to I2C and they are internally pulled up. This function is called from nau7802_setup();
 * 
 * \param scl_pin GPIO number serving as SCL pin
 * \param sda_pin GPIO number serving as SDA pin
 */
static void _nau7802_hw_init(uint8_t scl_pin, uint8_t sda_pin){
    i2c_init(_nau7802_i2c, 100 * 1000);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
}

/**
 * \brief Assign standard values to the registers of the NAU7802 chip.
 * 
 * \returns PICO_OK if setup successfully. Else returns error code.
 */
static int _nau7802_setup(){
    if(nau7802_reset()){
        return PICO_ERROR_GENERIC;
    }
    if(nau7802_set_digital_power(PWR_ON)){
        return PICO_ERROR_GENERIC;
    }
    if(nau7802_set_analog_power_supply(AVDD_SRC_INTERNAL)){
        return PICO_ERROR_GENERIC;
    }
    if(nau7802_set_analog_power(PWR_ON)){
        return PICO_ERROR_GENERIC;
    }
    if(nau7802_set_ldo_voltage(VLDO_3_0)){
        return PICO_ERROR_GENERIC;
    }
    if(nau7802_set_ldo_mode(LDO_MODE_ACCURATE)){
        return PICO_ERROR_GENERIC;
    }
    if(nau7802_set_gain(GAIN_128)){
        return PICO_ERROR_GENERIC;
    }
    if(nau7802_set_chopper_clock(CHP_CLK_OFF)){
        return PICO_ERROR_GENERIC;
    }
    if(nau7802_set_pga_filter(PGA_ON)){
        return PICO_ERROR_GENERIC;
    }
    if(nau7802_set_conversions(CONVERSIONS_ON)){
        return PICO_ERROR_GENERIC;
    }
    return PICO_OK;
}

int nau7802_setup(uint8_t scl_pin, uint8_t sda_pin, i2c_inst_t * nau7802_i2c, float conversion_factor_mg){
    if(nau7802_i2c != NULL){
        _nau7802_i2c = nau7802_i2c;
    } 
    _conversion_factor_mg = conversion_factor_mg;

    _nau7802_hw_init(scl_pin, sda_pin);

    // Try to setup scale up to ten times.
    for(int i = 0; i<10; i++){
        if(_nau7802_setup() == PICO_ERROR_NONE) break;
        if(i==9)return PICO_ERROR_GENERIC;
    }

    while(_scale_origin == 0){
        nau7802_zero();
    }
}