#ifndef NAU7802_HEADER_
#define NAU7802_HEADER_

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define ADDR_NAU7802 0x2A

#define NAU7802_SUCCESS 1
#define NAU7802_ERROR_WRITE_FAILURE -1
#define NAU7802_ERROR_READ_FAILURE  -2

typedef unsigned char byte;
typedef unsigned char reg_addr;

typedef const struct{
    const uint8_t from;
    const uint8_t to;
    const reg_addr in_reg;
} bit_range;

const reg_addr  REG_PU_CTRL;      //= 0x00;
const bit_range BITS_RESET;
const bit_range BITS_PWR_UP_D;
const bit_range BITS_PWR_UP_A;
const bit_range BITS_READY;
const bit_range BITS_CS;
const bit_range BITS_CR;
const bit_range BITS_OSCS;
const bit_range BITS_AVDD_S;

const reg_addr  REG_CTRL_1;       //= 0x01;
const bit_range BITS_GAIN;
const bit_range BITS_VLDO;
const bit_range BITS_DRDY_SEL;
const bit_range BITS_CRP;
 
const reg_addr  REG_CTRL_2;       //= 0x02;
const bit_range BITS_CAL_MODE;
const bit_range BITS_CALS;
const bit_range BITS_CAL_ERR;
const bit_range BITS_CRS;
const bit_range BITS_CHS;

const reg_addr  REG_I2C_CTRL;     //= 0x11;
const bit_range BITS_BGPCP;
const bit_range BITS_TS;
const bit_range BITS_BOPGA;
const bit_range BITS_SI;
const bit_range BITS_WPD;
const bit_range BITS_SPE;
const bit_range BITS_FRD;
const bit_range BITS_CRSD;

const reg_addr  REG_ADCO_B2;      //= 0x12;
const bit_range BITS_B23_16;

const reg_addr  REG_ADCO_B1;      //= 0x13;
const bit_range BITS_B15_08;

const reg_addr  REG_ADCO_B0;      //= 0x14;
const bit_range BITS_B07_00;

const reg_addr  REG_ADC_CTRL;     //= 0x15;
const bit_range BITS_REG_CHP;
const bit_range BITS_ADC_VCM;
const bit_range BITS_REG_CHPS;

const reg_addr  REG_PGA;          //= 0x1B;
const bit_range BITS_PGA_CHP_DIS;
const bit_range BITS_PGA_INV;
const bit_range BITS_PGA_BYP_EN;
const bit_range BITS_PGA_OBUF_EN;
const bit_range BITS_LDO_MODE;
const bit_range BITS_RD_OTP_SEL;

const reg_addr  REG_PWR_CTRL;     //= 0x1C;
const bit_range BITS_PGA_CURR;
const bit_range BITS_ADC_CURR;
const bit_range BITS_MST_BS_CURR;
const bit_range BITS_PGA_CAP;

const reg_addr  REG_DEV_REV;      //= 0x1F;
const bit_range BITS_REVISION_ID;
typedef enum _ldo_voltage{ 
    VLDO_2_4 = 0b111,
    VLDO_2_7 = 0b110,
    VLDO_3_0 = 0b101,
    VLDO_3_3 = 0b100,
    VLDO_3_6 = 0b011,
    VLDO_3_9 = 0b010,
    VLDO_4_2 = 0b001,
    VLDO_4_5 = 0b000,
    VLDO_DEFAULT = 0b000
} ldo_voltage;

typedef enum _gain{ 
    GAIN_128 = 0b111,
    GAIN_064 = 0b110,
    GAIN_032 = 0b101,
    GAIN_016 = 0b100,
    GAIN_008 = 0b011,
    GAIN_004 = 0b010,
    GAIN_002 = 0b001,
    GAIN_001 = 0b000,
    GAIN_DEFAULT = 0b000
} gain;

typedef enum _conversion_rate{
    SPS_320 = 0b111,
    SPS_080 = 0b011,
    SPS_040 = 0b010,
    SPS_020 = 0b001,
    SPS_010 = 0b000,
    SPS_DEFAULT = 0b000
} conversion_rate;

typedef enum _calibration_mode{
    MODE_GAIN_SYS= 0b11,
    MODE_OFF_SYS = 0b10,
    MODE_OFF_INT = 0b00,
    MODE_DEFAULT = 0b00
} calibration_mode;

typedef enum _master_bias_curr{ 
    BIAS_CURR_054 = 0b111,
    BIAS_CURR_058 = 0b110,
    BIAS_CURR_062 = 0b101,
    BIAS_CURR_067 = 0b100,
    BIAS_CURR_073 = 0b011,
    BIAS_CURR_080 = 0b010,
    BIAS_CURR_090 = 0b001,
    BIAS_CURR_100 = 0b000,
    BIAS_CURR_DEFAULT = 0b000
} master_bias_curr;

// Current for the ADC circuit. Percent of master bias.
typedef enum _adc_curr{ 
    ADC_CURR_025 = 0b11,
    ADC_CURR_050 = 0b10,
    ADC_CURR_075 = 0b01,
    ADC_CURR_100 = 0b00,
    ADC_CURR_DEFAULT = 0b00
} adc_curr;

// Current for PGA system. Percent of master bias. 
typedef enum _pga_curr{ 
    PGA_CURR_070 = 0b11,
    PGA_CURR_086 = 0b10,
    PGA_CURR_095 = 0b01,
    PGA_CURR_100 = 0b00,
    PGA_CURR_DEFAULT = 0b00
} pga_curr;

typedef enum _avdd_src{
    AVDD_SRC_INTERNAL = 1,
    AVDD_SRC_PIN = 0,
    AVDD_SRC_DEFAULT  = 0
} avdd_src;

typedef enum _pwr_setting{
    PWR_ON = 1,
    PWR_OFF = 0,
    PWR_DEFAULT = 0
} pwr_setting;

typedef enum _ldo_mode{
    LDO_MODE_STABLE = 1,
    LDO_MODE_ACCURATE = 0,
    LDO_MODE_DEFAULT = 0
} ldo_mode;

typedef enum _chp_clk{
    CHP_CLK_OFF = 0b11,
    CHP_CLK_DEFAULT = 0b11,
} chp_clk;

typedef enum _conversion_setting{
    CONVERSIONS_ON = 1,
    CONVERSIONS_OFF = 0,
    CONVERSIONS_DEFAULT = 0
} conversion_setting;

typedef enum _pga_setting{
    PGA_ON = 1,
    PGA_OFF = 0,
} pga_setting;

/**
 * \brief Read the nau7802 registers from reg_idx to reg_idx+len and store them in dst. 
 * 
 * \param reg_idx Starting register index on NAU7802 to read from.
 * \param len     Number of registers to read. dst must point to a buffer of this size. 
 * \param dst     Location to store register values.
 * 
 * \return NAU7802_SUCCESS if operation is completed successfully, NAU7802_ERROR_WRITE_FAILURE if write 
 * operation failed and NAU7802_ERROR_READ_FAILURE if read operation failed. 
 */
int nau7802_read_reg(const reg_addr reg_idx, uint8_t len, uint8_t * dst);

/**
 * \brief Write from src array to nau7802 registers ranging from reg_idx to reg_idx+len. 
 * 
 * \param reg_idx Starting register index on NAU7802 to read from.
 * \param len     Number of registers to read. dst must point to a buffer of this size. 
 * \param src     Pointer to values to place in targeted registers
 * 
 * \return NAU7802_SUCCESS if operation is completed successfully and 
 * NAU7802_ERROR_WRITE_FAILURE if write operation failed. 
 */
int nau7802_write_reg(const reg_addr reg_idx, uint8_t len, uint8_t * src);

/**
 * \brief Read the bits in reg idx specified by bit_range bits into dst.
 * 
 * \param bits A bit_range struct indicating the register address and bits to read.
 * \param dst A pointer to the target location where the bits are stored.
 * 
 * \return NAU7802_SUCCESS if successfull and an error code otherwise. 
 */
int nau7802_read_bits(const bit_range bits, uint8_t * dst);

/**
 * \brief Write a val to specific bits in reg idx specified by bit_range bits.
 * 
 * \param bits A bit_range struct indicating the register address and bits to read.
 * \param val A pointer to the value that will be written to the bits. 
 * 
 * \return NAU7802_SUCCESS if successfull and an error code otherwise. 
 */
int nau7802_write_bits(const bit_range bits, uint8_t val);

/** 
 * \brief Sets and unsets the reset bit in the NAU7802. This clears the registers
 * and returns the chip to a known state. Setup operations must be repeated after
 * reset. 
 */
void nau7802_reset();

/**
 * \brief Checks if the bit indicating the NAU7802 is ready is set.
 * 
 * \return True if NAU7802 ready bit is set. False otherwise. 
 */
bool nau7802_is_ready();

/**
 * \brief Select the source of the analog power. This can either come from an onboard
 * regulator or externally. 
 * 
 * \param source Enumerated value indicating where to source the analog power. Options are
 * AVDD_SRC_INTERNAL, AVDD_SRC_PIN, and AVDD_SRC_DEFAULT (i.e. AVDD_SRC_PIN)
 */
void nau7802_set_analog_power_supply(avdd_src source);

/**
 * \brief Activate or disable the digital logic on the NAU7802.  
 * 
 * \param on_off Enumerated power setting. Options are PWR_ON and PWR_OFF.
 */
void nau7802_set_digital_power(pwr_setting on_off);

/**
 * \brief Activate or disable the analog circuit on the NAU7802.  
 * 
 * \param on_off Enumerated power setting. Options are PWR_ON and PWR_OFF.
 */
void nau7802_set_analog_power(pwr_setting on_off);

/**
 * \brief Start or stop the ADC conversions.
 * 
 * \param on_off Enumerated conversion settings. Options are CONVERSIONS_ON and CONVERSIONS_OFF.
 */
void nau7802_set_conversions(conversion_setting on_off);

/**
 * \brief Set the gain of the ADC process in the NAU7802.
 * 
 * \param g Enumerated value indicating the gain. Options are GAIN_001, GAIN_002,..., GAIN_128.
 */
void nau7802_set_gain(gain g);

/**
 * \brief Set the LDO voltage in the NAU7802.
 * 
 * \param v Enumerated value indicating the LDO voltage. Values are in the form VLDO_X_Y for X.Y volts.
 */
void nau7802_set_ldo_voltage(ldo_voltage v);

/**
 * \brief Set the LDO mode in the NAU7802.
 * 
 * \param mode Enumerated value indicating the LDO mode. Options are LDO_MODE_STABLE and LDO_MODE_ACCURATE.
 */
void nau7802_set_ldo_mode(ldo_mode mode);

/**
 * \brief Set the chopper clock on the NAU7802. Turned off is the only setting in the NAU7802.
 * 
 * \param val PGA filter setting. Only can be CHP_CLK_OFF.
 */
void nau7802_set_chopper_clock(chp_clk val);

/**
 * \brief Enable or disable the PGA filter in the NAU7802.
 * 
 * \param off_on PGA filter setting. Options are PGA_ON and PGA_OFF. 
 */
void nau7802_set_pga_filter(pga_setting off_on);

/**
 * \brief Check if a new data conversion is ready in the NAU7802.
 * 
 * \return True if conversion is ready. False else. 
 */
bool nau7802_data_ready();

/**
 * \brief Read the latest conversion result into dst. If no conversion has ever been read, 0 is returned.
 * 
 * \param dst Pointer to 32 bit buffer to store conversion result. 
 */
void nau7802_read(uint32_t * dst);

/**
 * Initalize HW and set NAU7802 registers to the default values:
 * Internal analog source,
 * Digital power on,
 * Analog power on,
 * 3.0V LDO voltage,
 * LDO in accurate mode,
 * Amplifier gain set to 128,
 * Chop clock off,
 * Activate PGA filter,
 * Start conversions.
 * 
 * \param scl_pin GPIO number serving as SCL pin
 * \param sda_pin GPIO number serving as SDA pin
 * \param nau7802_i2c pointer to desired I2C instance. If NULL, default I2C instane is used.
 */
void nau7802_setup(uint8_t scl_pin, uint8_t sda_pin, i2c_inst_t * nau7802_i2c);
#endif