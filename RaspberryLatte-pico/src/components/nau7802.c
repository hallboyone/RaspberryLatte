#include <stdio.h>
#include <string.h>

#include "nau7802.h"
#include "uart_bridge.h"
#include "status_ids.h"

const reg_addr  REG_PU_CTRL      = 0x00;
const bit_range BITS_RESET       = {.from = 0, .to = 0, .in_reg = REG_PU_CTRL};
const bit_range BITS_PWR_UP_D    = {.from = 1, .to = 1, .in_reg = REG_PU_CTRL};
const bit_range BITS_PWR_UP_A    = {.from = 2, .to = 2, .in_reg = REG_PU_CTRL};
const bit_range BITS_READY       = {.from = 3, .to = 3, .in_reg = REG_PU_CTRL};
const bit_range BITS_CS          = {.from = 4, .to = 4, .in_reg = REG_PU_CTRL};
const bit_range BITS_CR          = {.from = 5, .to = 5, .in_reg = REG_PU_CTRL};
const bit_range BITS_OSCS        = {.from = 6, .to = 6, .in_reg = REG_PU_CTRL};
const bit_range BITS_AVDD_S      = {.from = 7, .to = 7, .in_reg = REG_PU_CTRL};

const reg_addr  REG_CTRL_1       = 0x01;
const bit_range BITS_GAIN        = {.from = 0, .to = 2, .in_reg = REG_CTRL_1};
const bit_range BITS_VLDO        = {.from = 3, .to = 5, .in_reg = REG_CTRL_1};
const bit_range BITS_DRDY_SEL    = {.from = 6, .to = 6, .in_reg = REG_CTRL_1};
const bit_range BITS_CRP         = {.from = 7, .to = 7, .in_reg = REG_CTRL_1};

const reg_addr  REG_CTRL_2       = 0x02;
const bit_range BITS_CAL_MODE    = {.from = 0, .to = 1, .in_reg = REG_CTRL_2};
const bit_range BITS_CALS        = {.from = 2, .to = 2, .in_reg = REG_CTRL_2};
const bit_range BITS_CAL_ERR     = {.from = 3, .to = 3, .in_reg = REG_CTRL_2};
const bit_range BITS_CRS         = {.from = 4, .to = 6, .in_reg = REG_CTRL_2};
const bit_range BITS_CHS         = {.from = 7, .to = 7, .in_reg = REG_CTRL_2};

const reg_addr  REG_I2C_CTRL     = 0x11;
const bit_range BITS_BGPCP       = {.from = 0, .to = 0, .in_reg = REG_I2C_CTRL};
const bit_range BITS_TS          = {.from = 1, .to = 1, .in_reg = REG_I2C_CTRL};
const bit_range BITS_BOPGA       = {.from = 2, .to = 2, .in_reg = REG_I2C_CTRL};
const bit_range BITS_SI          = {.from = 3, .to = 3, .in_reg = REG_I2C_CTRL};
const bit_range BITS_WPD         = {.from = 4, .to = 4, .in_reg = REG_I2C_CTRL};
const bit_range BITS_SPE         = {.from = 5, .to = 5, .in_reg = REG_I2C_CTRL};
const bit_range BITS_FRD         = {.from = 6, .to = 6, .in_reg = REG_I2C_CTRL};
const bit_range BITS_CRSD        = {.from = 7, .to = 7, .in_reg = REG_I2C_CTRL};
    
const reg_addr  REG_ADCO_B2      = 0x12;
const bit_range BITS_B23_16      = {.from = 0, .to = 7, .in_reg = REG_ADCO_B2};
    
const reg_addr  REG_ADCO_B1      = 0x13;
const bit_range BITS_B15_08      = {.from = 0, .to = 7, .in_reg = REG_ADCO_B1};

const reg_addr  REG_ADCO_B0      = 0x14;
const bit_range BITS_B07_00      = {.from = 0, .to = 7, .in_reg = REG_ADCO_B0};

const reg_addr  REG_ADC_CTRL     = 0x15;
const bit_range BITS_REG_CHP     = {.from = 0, .to = 1, .in_reg = REG_ADC_CTRL};
const bit_range BITS_ADC_VCM     = {.from = 2, .to = 3, .in_reg = REG_ADC_CTRL};
const bit_range BITS_REG_CHPS    = {.from = 4, .to = 5, .in_reg = REG_ADC_CTRL};

const reg_addr  REG_PGA          = 0x1B;
const bit_range BITS_PGA_CHP_DIS = {.from = 0, .to = 0, .in_reg = REG_PGA};
const bit_range BITS_PGA_INV     = {.from = 3, .to = 3, .in_reg = REG_PGA};
const bit_range BITS_PGA_BYP_EN  = {.from = 4, .to = 4, .in_reg = REG_PGA};
const bit_range BITS_PGA_OBUF_EN = {.from = 5, .to = 5, .in_reg = REG_PGA};
const bit_range BITS_LDO_MODE    = {.from = 6, .to = 6, .in_reg = REG_PGA};
const bit_range BITS_RD_OTP_SEL  = {.from = 7, .to = 7, .in_reg = REG_PGA};

const reg_addr  REG_PWR_CTRL     = 0x1C;
const bit_range BITS_PGA_CURR    = {.from = 0, .to = 1, .in_reg = REG_PWR_CTRL};
const bit_range BITS_ADC_CURR    = {.from = 2, .to = 3, .in_reg = REG_PWR_CTRL};
const bit_range BITS_MST_BS_CURR = {.from = 4, .to = 6, .in_reg = REG_PWR_CTRL};
const bit_range BITS_PGA_CAP     = {.from = 7, .to = 7, .in_reg = REG_PWR_CTRL};

const reg_addr  REG_DEV_REV      = 0x1F;
const bit_range BITS_REVISION_ID = {.from = 0, .to = 3, .in_reg = REG_DEV_REV};

/**
 * \brief Last ADC reading.
 */
static uint32_t last_val = 0;

/**
 * \brief I2C Channel to use for the NAU7802.
 */
static i2c_inst_t * _nau7802_i2c = i2c_default;

/**
 * \brief Returns the bits within buf specified by the bit_range bits.
 * 
 * \param buf Byte storing packed binary data.
 * \param bits A bit_range structure indicating the bits to extract.
 * 
 * \returns The targeted bits in buf with the LS target bit shifted to bit 0.
 */
inline uint8_t extractBits(const byte buf, const bit_range bits){
    return (buf<<(7-bits.to))>>(7-bits.to + bits.from);
}

/**
 * \brief Sets the bits within buf specified by the bit_range bits. Only the specified bits
 * are ever written to. If the value does not fit within the specified bits, the highest 
 * bits are truncated.
 * 
 * \param buf  Byte storing packed binary data.
 * \param bits A bit_range structure indicating the bits to extract.
 * \param val  The value to store in targeted bits. 
 * 
 * \return The original buf with the targeted bits overwritten with the val. 
 */
inline uint8_t setBits(const byte buf, const bit_range bits, uint8_t val){
    const uint8_t val_mask = 0xFFu>>(7-(bits.to - bits.from));
    const uint8_t buf_mask = ~(val_mask<<bits.from);
    return (buf & buf_mask) | ((val & val_mask))<<bits.from;
}

int nau7802_read_reg(const reg_addr reg_idx, uint8_t len, uint8_t * dst){
    // Send the starting register address
    if(i2c_write_blocking(_nau7802_i2c, ADDR_NAU7802, &reg_idx, 1, true) == PICO_ERROR_GENERIC){
        printf("WARNING: Write operation failed! [nau7802_read_reg]\n");
        return NAU7802_ERROR_WRITE_FAILURE;
    }
    // Read each register into dst 
    if(i2c_read_blocking(_nau7802_i2c, ADDR_NAU7802, dst, len, false) == PICO_ERROR_GENERIC){
        printf("WARNING: Read operation failed! [nau7802_read_reg]\n");
        return NAU7802_ERROR_READ_FAILURE;
    }
    return NAU7802_SUCCESS;
}

int nau7802_write_reg(const reg_addr reg_idx, uint8_t len, uint8_t * src){
    uint8_t buf [len+1];
    buf[0] = reg_idx;
    memcpy(&buf[1], src, len);
    if(i2c_write_blocking(_nau7802_i2c, ADDR_NAU7802, buf, len+1, false) == PICO_ERROR_GENERIC){
        printf("WARNING: Write operation failed! [nau7802Write2Bit]\n");
        return NAU7802_ERROR_WRITE_FAILURE;
    }
    return NAU7802_SUCCESS;
}

int nau7802_read_bits(const bit_range bits, uint8_t * dst){
    int err_code = 0;
    if((err_code=nau7802_read_reg(bits.in_reg, 1, dst)) != NAU7802_SUCCESS){
        return err_code;
    }
    *dst = extractBits(*dst, bits);
    return NAU7802_SUCCESS;
}

int nau7802_write_bits(const bit_range bits, uint8_t val){
    byte reg = 0;
    int err_code = 0;
    if((err_code=nau7802_read_reg(bits.in_reg, 1, &reg)) != NAU7802_SUCCESS){
        return err_code;
    }
    setBits(reg, bits, val);
    if((err_code=nau7802_write_reg(bits.in_reg, 1, &reg)) != NAU7802_SUCCESS){
        return err_code;
    }
    return NAU7802_SUCCESS;
}

void nau7802_reset(){
    if(nau7802_write_bits(BITS_RESET, 1) != NAU7802_SUCCESS){
        printf("Failed to set the reset bit to 1\n");
    }
    sleep_ms(1);
    if(nau7802_write_bits(BITS_RESET, 0) != NAU7802_SUCCESS){
        printf("Failed to set the reset bit to 0\n");
    }
}

bool nau7802_is_ready(){
    uint8_t ready_bit = 0;
    if(nau7802_read_bits(BITS_READY, &ready_bit) != NAU7802_SUCCESS){
        printf("Failed to get ready bits\n");
    }
    return ready_bit;
}

void nau7802_set_analog_power_supply(avdd_src source){
    if(nau7802_write_bits(BITS_AVDD_S, source) != NAU7802_SUCCESS){
        printf("Failed to set the analog power supply\n");
    }
}

void nau7802_set_digital_power(pwr_setting on_off){
    if(nau7802_write_bits(BITS_PWR_UP_D, on_off) != NAU7802_SUCCESS){
        printf("Failed to set the digital power\n");
    }
    if (on_off){ // If powering up, wait for it to be ready.
        while(!nau7802_is_ready()){
            tight_loop_contents;
        }
    }
}

void nau7802_set_analog_power(pwr_setting on_off){
    if(nau7802_write_bits(BITS_PWR_UP_A, on_off) != NAU7802_SUCCESS){
        printf("Failed to set the analog power\n");
    }
}

void nau7802_set_conversions(conversion_setting on_off){
    if(nau7802_write_bits(BITS_CS, on_off) != NAU7802_SUCCESS){
        printf("Failed to set conversions\n");
    }
}

void nau7802_set_gain(gain g){
    if(nau7802_write_bits(BITS_GAIN, g) != NAU7802_SUCCESS){
        printf("Failed to update gain\n");
    }
}

void nau7802_set_ldo_voltage(ldo_voltage v){
    if(nau7802_write_bits(BITS_VLDO, v) != NAU7802_SUCCESS){
        printf("Failed to update LDO voltage\n");
    }
}

void nau7802_set_ldo_mode(ldo_mode mode){
    if(nau7802_write_bits(BITS_LDO_MODE, mode) != NAU7802_SUCCESS){
        printf("Failed to update LDO mode\n");
    }
}

void nau7802_set_chopper_clock(chp_clk val){
    if(nau7802_write_bits(BITS_REG_CHPS, val) != NAU7802_SUCCESS){
        printf("Failed to update chopper clock\n");
    }
}

void nau7802_set_pga_filter(pga_setting off_on){
    if(nau7802_write_bits(BITS_PGA_CAP, off_on) != NAU7802_SUCCESS){
        printf("Failed to update pga filter cap status\n");
    }
}

bool nau7802_data_ready(){
    uint8_t is_ready = 0;
    if(nau7802_read_bits(BITS_CR, &is_ready) != NAU7802_SUCCESS){
        printf("Failed to get conversion status\n");
    }
    return (is_ready==1);
}

void nau7802_read(uint32_t * dst){
    if (nau7802_data_ready()){
        nau7802_read_reg(REG_ADCO_B2, 1, ((uint8_t *) dst)+2);
        nau7802_read_reg(REG_ADCO_B1, 1, ((uint8_t *) dst)+1);
        nau7802_read_reg(REG_ADCO_B0, 1, ((uint8_t *) dst));
        last_val = *dst;
    } else {
        *dst = last_val;
    }
}

/**
 * \brief Respond to MSG_ID_GET_WEIGHT request with latest ADC conversion value.
 * 
 * \param msg Pointer to message
 * \param len Length of message
 */
static void _nau7802_read_handler(int * msg, int len){
    if(len==1){
        uint32_t val = 0;
        nau7802_read(&val);
        int response [3] = {(val >> 16) & 0xFF, (val >>  8) & 0xFF, (val >>  0) & 0xFF};
        sendMessageWithStatus(MSG_ID_GET_WEIGHT, MSG_READ_SUCCESS, response, 3);
    } else {
        sendMessageWithStatus(MSG_ID_SET_HEATER, MSG_FORMAT_ERROR, NULL, 0);
    }
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

void nau7802_setup(uint8_t scl_pin, uint8_t sda_pin, i2c_inst_t * nau7802_i2c){
    if(nau7802_i2c != NULL){
        _nau7802_i2c = nau7802_i2c;
    }
    _nau7802_hw_init(scl_pin, sda_pin);

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

    registerHandler(MSG_ID_GET_WEIGHT, &_nau7802_read_handler);
}