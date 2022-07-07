#include "nau7802.h"

#include "hardware/i2c.h"

#include <string.h>

static uint32_t last_val = 0;

/**
 * ====================================================================
 *               Bit manipulation and display functions 
 * ====================================================================
 */

void printBits(byte b){
    for(int i=0; i<8; i++){
        if(b & 0x80){
            printf("1");
        } else {
            printf("0");
        }
        b <<= 1;
    }
    printf("\n");
}

/**
 * Returns the bits within buf specified by the bit_range bits.
 * Ex: extractBits(0b01101001, {.from=2, .to=5}) = 0b1010
 */
inline uint8_t extractBits(const byte buf, const bit_range bits){
    return (buf<<(7-bits.to))>>(7-bits.to + bits.from);
}

/**
 * Sets the bits within buf specified by the bit_range bits. If the value is too large 
 * to fit in specified bits, the highest bits are truncated to not overwrite higher bits
 * in the byte.
 * Ex: setBits(0b01101001, {.from=2, .to=5}, 0b0101) = 0b01010101
 */
inline uint8_t setBits(const byte buf, const bit_range bits, uint8_t val){
    uint8_t val_mask = 0xFFu>>(7-(bits.to - bits.from));
    uint8_t buf_mask = ~(val_mask<<bits.from);
    return (buf & buf_mask) | ((val & val_mask))<<bits.from;
}

/**
 * ====================================================================
 *                Register getter and setter functions
 * ====================================================================
 */
static i2c_inst_t * nau7802_i2c = i2c_default;

/**
 * Read the nau7802 registers from reg_idx to reg_idx+len and store them in dst. Returns NAU7802_SUCCESS
 * if operation is completed successfully, NAU7802_ERROR_WRITE_FAILURE if write operation failed and 
 * NAU7802_ERROR_READ_FAILURE if read operation failed. 
 */
int nau7802_read_reg(const reg_addr reg_idx, uint8_t * dst, uint8_t len){
    // Send the starting register address
    if(i2c_write_blocking(nau7802_i2c, ADDR_NAU7802, &reg_idx, 1, true) == PICO_ERROR_GENERIC){
        printf("WARNING: Write operation failed! [nau7802_read_reg]\n");
        return NAU7802_ERROR_WRITE_FAILURE;
    }
    // Read each register into dst 
    if(i2c_read_blocking(nau7802_i2c, ADDR_NAU7802, dst, len, false) == PICO_ERROR_GENERIC){
        printf("WARNING: Read operation failed! [nau7802_read_reg]\n");
        return NAU7802_ERROR_READ_FAILURE;
    }
    return NAU7802_SUCCESS;
}

/**
 * Write from src array to nau7802 registers ranging from reg_idx to reg_idx+len. Returns NAU7802_SUCCESS
 * if operation is completed successfully and NAU7802_ERROR_WRITE_FAILURE if write operation failed. 
 */
int nau7802_write_reg(const reg_addr reg_idx, uint8_t * src, uint8_t len){
    uint8_t buf [len+1];
    buf[0] = reg_idx;
    memcpy(buf[1], src, len);
    if(i2c_write_blocking(nau7802_i2c, ADDR_NAU7802, buf, len+1, false) == PICO_ERROR_GENERIC){
        printf("WARNING: Write operation failed! [nau7802Write2Bit]\n");
        return NAU7802_ERROR_WRITE_FAILURE;
    }
    return NAU7802_SUCCESS;
}

/**
 * Read the bits in reg idx specified by bit_range bits into dst. Returns NAU7802_SUCCESS if
 * successfull and an error code otherwise. 
 */
int nau7802_read_bits(const bit_range bits, uint8_t * dst){
    int err_code = 0;
    if((err_code=nau7802_read_reg(bits.in_reg, dst, 1)) != NAU7802_SUCCESS){
        return err_code;
    }
    *dst = extractBits(*dst, bits);
    return NAU7802_SUCCESS;
}

/**
 * Write a val to specific bits in reg idx specified by bit_range bits. Returns NAU7802_SUCCESS if
 * successfull and an error code otherwise. 
 */
int nau7802_write_bits(const bit_range bits, uint8_t val){
    byte reg = 0;
    int err_code = 0;
    if((err_code=nau7802_read_reg(bits.in_reg, &reg, 1)) != NAU7802_SUCCESS){
        return err_code;
    }
    setBits(reg, bits, val);
    if((err_code=nau7802_write_reg(bits.in_reg, &reg, 1)) != NAU7802_SUCCESS){
        return err_code;
    }
    return NAU7802_SUCCESS;
}

/**
 * ====================================================================
 *                      NAU7802 Basic Operations
 * ====================================================================
 */
int nau7802_reset(){
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

void nau7802_start_conversions(){
    if(nau7802_write_bits(BITS_CS, 1) != NAU7802_SUCCESS){
        printf("Failed to start conversions\n");
    }
}

void nau7802_stop_conversions(){
    if(nau7802_write_bits(BITS_CS, 0) != NAU7802_SUCCESS){
        printf("Failed to stop conversions\n");
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

void nau7802_set_pga_filter(bool off_on){
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
/**
 * ====================================================================
 *                     NAU7802 Setup and Reading
 * ====================================================================
 */

void nau7802_hw_init(){
    i2c_init(nau7802_i2c, 100 * 1000);
    gpio_set_function(NAU7802_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(NAU7802_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(NAU7802_SDA_PIN);
    gpio_pull_up(NAU7802_SCL_PIN);
}

void nau7802_setup(){
    nau7802_hw_init();

    nau7802_reset();
    nau7802_set_analog_power_supply(AVDD_SRC_INTERNAL);
    nau7802_set_digital_power(PWR_ON);
    nau7802_set_analog_power(PWR_ON);
    
    nau7802_set_ldo_voltage(VLDO_3_0);
    nau7802_set_ldo_mode(LDO_MODE_ACCURATE);
    nau7802_set_gain(GAIN_128);
    nau7802_set_chopper_clock(CHP_CLK_OFF);
    nau7802_set_pga_filter(true);

    nau7802_start_conversions();
}

void nau7802_read(uint32_t * dst){
    if (nau7802_data_ready()){
        nau7802_read_reg(REG_ADCO_B2, ((uint8_t *) dst)+2, 1);
        nau7802_read_reg(REG_ADCO_B1, ((uint8_t *) dst)+1, 1);
        nau7802_read_reg(REG_ADCO_B0, ((uint8_t *) dst)  , 1);
        last_val = *dst;
    } else {
        *dst = last_val;
    }
}