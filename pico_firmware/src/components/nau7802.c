#include <stdio.h>
#include <string.h>

#include "nau7802.h"

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

void nau7802_reset(){
    if(nau7802_write_bits(BITS_RESET, 1) != I2C_BUS_SUCCESS){
        printf("Failed to set the reset bit to 1\n");
    }
    sleep_ms(1);
    if(nau7802_write_bits(BITS_RESET, 0) != I2C_BUS_SUCCESS){
        printf("Failed to set the reset bit to 0\n");
    }
}

bool nau7802_is_ready(){
    uint8_t ready_bit = 0;
    if(nau7802_read_bits(BITS_READY, &ready_bit) != I2C_BUS_SUCCESS){
        printf("Failed to get ready bits\n");
    }
    return ready_bit;
}

void nau7802_set_analog_power_supply(avdd_src source){
    if(nau7802_write_bits(BITS_AVDD_S, source) != I2C_BUS_SUCCESS){
        printf("Failed to set the analog power supply\n");
    }
}

void nau7802_set_digital_power(pwr_setting on_off){
    if(nau7802_write_bits(BITS_PWR_UP_D, on_off) != I2C_BUS_SUCCESS){
        printf("Failed to set the digital power\n");
    }
    byte reg = 0;
    nau7802_read_reg(BITS_PWR_UP_D.in_reg, 1, &reg);
    if (on_off){ // If powering up, wait for it to be ready.
        while(!nau7802_is_ready()){
            tight_loop_contents;
        }
    }
}

void nau7802_set_analog_power(pwr_setting on_off){
    if(nau7802_write_bits(BITS_PWR_UP_A, on_off) != I2C_BUS_SUCCESS){
        printf("Failed to set the analog power\n");
    }
}

void nau7802_set_conversions(conversion_setting on_off){
    if(nau7802_write_bits(BITS_CS, on_off) != I2C_BUS_SUCCESS){
        printf("Failed to set conversions\n");
    }
}

void nau7802_set_gain(gain g){
    if(nau7802_write_bits(BITS_GAIN, g) != I2C_BUS_SUCCESS){
        printf("Failed to update gain\n");
    }
}

void nau7802_set_ldo_voltage(ldo_voltage v){
    if(nau7802_write_bits(BITS_VLDO, v) != I2C_BUS_SUCCESS){
        printf("Failed to update LDO voltage\n");
    }
}

void nau7802_set_ldo_mode(ldo_mode mode){
    if(nau7802_write_bits(BITS_LDO_MODE, mode) != I2C_BUS_SUCCESS){
        printf("Failed to update LDO mode\n");
    }
}

void nau7802_set_chopper_clock(chp_clk val){
    if(nau7802_write_bits(BITS_REG_CHPS, val) != I2C_BUS_SUCCESS){
        printf("Failed to update chopper clock\n");
    }
}

void nau7802_set_pga_filter(pga_setting off_on){
    if(nau7802_write_bits(BITS_PGA_CAP, off_on) != I2C_BUS_SUCCESS){
        printf("Failed to update pga filter cap status\n");
    }
}

bool nau7802_data_ready(){
    uint8_t is_ready = 0;
    if(nau7802_read_bits(BITS_CR, &is_ready) != I2C_BUS_SUCCESS){
        printf("Failed to get conversion status\n");
    }
    return (is_ready==1);
}

void nau7802_read_raw(uint32_t * dst){
    if (nau7802_data_ready()){
        nau7802_read_reg(REG_ADCO_B2, 3, (uint8_t*)dst);
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
    nau7802_read_raw(&val);
    return _conversion_factor_mg*((int)val-(int)_scale_origin);
}

void nau7802_zero(){
    nau7802_read_raw(&_scale_origin);
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

void nau7802_setup(uint8_t scl_pin, uint8_t sda_pin, i2c_inst_t * nau7802_i2c, float conversion_factor_mg){
    if(nau7802_i2c != NULL){
        _nau7802_i2c = nau7802_i2c;
    } 
    _conversion_factor_mg = conversion_factor_mg;

    i2c_bus_setup(_nau7802_i2c, 100000, scl_pin, sda_pin);

    nau7802_reset();
    nau7802_set_analog_power_supply(AVDD_SRC_INTERNAL);
    nau7802_set_digital_power(PWR_ON);
    nau7802_set_analog_power(PWR_ON);
    
    nau7802_set_ldo_voltage(VLDO_3_0);
    nau7802_set_ldo_mode(LDO_MODE_ACCURATE);
    nau7802_set_gain(GAIN_128);
    nau7802_set_chopper_clock(CHP_CLK_OFF);
    nau7802_set_pga_filter(PGA_ON);

    nau7802_set_conversions(CONVERSIONS_ON);

    while(_scale_origin == 0){
        nau7802_zero();
    }
}