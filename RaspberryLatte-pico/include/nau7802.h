#include "pico/stdlib.h"

#define ADDR_NAU7802 0x2A

#define NAU7802_SCL_PIN PICO_DEFAULT_I2C_SCL_PIN
#define NAU7802_SDA_PIN PICO_DEFAULT_I2C_SCL_PIN

typedef unsigned char byte;
typedef unsigned char reg_addr;

typedef const struct{
    const uint8_t from;
    const uint8_t to;
    const reg_addr in_reg;
} bit_range;

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

#define ON true
#define OFF false
#define START true
#define STOP false
#define INTERNAL true
#define EXTERNAL false

#define NAU7802_SUCCESS 1
#define NAU7802_ERROR_WRITE_FAILURE -1
#define NAU7802_ERROR_READ_FAILURE  -2