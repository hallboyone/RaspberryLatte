#include "i2c_bus.h"

void i2c_bus_set_bits(byte* buf, uint8_t from_bit, uint8_t to_bit, uint8_t val){
    const uint8_t val_mask = 0xFFu>>(7-(to_bit - from_bit));
    const uint8_t buf_mask = ~(val_mask<<from_bit);
    return ((*buf) & buf_mask) | ((val & val_mask))<<from_bit;
}

int i2c_bus_read_bytes(i2c_inst_t * bus, dev_addr dev, reg_addr reg, uint8_t reg_addr_len, uint len, byte * dst){
    if(i2c_write_blocking(bus, dev, (uint8_t *)(&reg), reg_addr_len, true) == PICO_ERROR_GENERIC){
        return I2C_ERROR_WRITE_FAILURE;
    }
    // Read each register into dst 
    if(i2c_read_blocking(bus, dev, dst, len, false) == PICO_ERROR_GENERIC){
        return I2C_ERROR_READ_FAILURE;
    }
    return I2C_SUCCESS;
}

int i2c_bus_write_bytes(i2c_inst_t * bus, dev_addr dev, reg_addr reg, uint8_t reg_addr_len, uint len, byte * src){
    if(i2c_write_blocking(bus, dev, (uint8_t *)&reg, reg_addr_len, true) == PICO_ERROR_GENERIC){
        return I2C_ERROR_WRITE_FAILURE;
    }
    if(i2c_write_blocking(bus, dev, src, len, false) == PICO_ERROR_GENERIC){
        return I2C_ERROR_WRITE_FAILURE;
    }
    return I2C_SUCCESS;
}

int i2c_bus_read_bits(i2c_inst_t * bus, dev_addr dev, reg_addr reg, uint8_t reg_addr_len, 
                      uint8_t from_bit, uint8_t to_bit, byte * dst){
    // Read specified register from device
    int result = i2c_bus_read_bytes(bus, dev, reg, reg_addr_len, 1, dst);
    if(result != I2C_SUCCESS) return result;

    // Shift bits to only include those in range.
    *dst = (*dst)<<(7-to_bit);
    (*dst) = (*dst)>>(7-to_bit + from_bit);

    return I2C_SUCCESS;
}

int i2c_bus_write_bits(i2c_inst_t * bus, dev_addr dev, reg_addr reg, uint8_t reg_addr_len, uint8_t from_bit, uint8_t to_bit, byte val){
    byte reg = 0;
    int result = i2c_bus_read_bytes(bus, dev, reg, reg_addr_len, 1, &reg);
    if(result != I2C_SUCCESS) return result;
    
    i2c_bus_set_bits(&reg, from_bit, to_bit, val);

    result = i2c_bus_write_bytes(bus, dev, reg, reg_addr_len, 1, &reg);
    if(result != I2C_SUCCESS) return result;
    
    return I2C_SUCCESS;
}