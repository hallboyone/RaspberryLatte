/**
 * \file 
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Functions for communicating over an I2C bus
 * \version 0.1
 * \date 2022-08-17
 */

#ifndef I2C_BUS_H
#define I2C_BUS_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C_SUCCESS 0              /**< Code used to indicate a successfull I2C operations */
#define I2C_ERROR_WRITE_FAILURE -1 /**< Code used to indicate a failed I2C write operation */
#define I2C_ERROR_READ_FAILURE  -2 /**< Code used to indicate a failed I2C read operation */

typedef uint8_t byte;           /**< Type name for a single byte of data */
typedef uint8_t dev_addr;       /**< Type name for a device address */
typedef uint32_t reg_addr;      /**< Type name for up to a 32 bit register address. */

/**
 * \brief Sets the bits within buf specified by the from_bit and to_bit. Only the specified bits
 * are ever written to. If the value does not fit within the specified bits, the highest 
 * bits are truncated.
 * 
 * \param buf Pointer to byte that will be updated
 * \param from_bit The LSB that will be overwritten
 * \param to_bit The MSB that will be overwritten
 * \param val The value to pack into byte. Bit 0 is set to the from_bit and so on.
 */
void i2c_bus_set_bits(byte* buf, uint8_t from_bit, uint8_t to_bit, uint8_t val);

/**
 * \brief Read bytes from a register on a device connected to a I2C bus.
 * 
 * \param bus The I2C instance the device is attached to.
 * \param dev The device address. must be less than 0x80.
 * \param reg The register address on the device.
 * \param reg_addr_len The length of the address in bytes. Allows up to 4 byte (32bit) address spaces
 * \param len The number of registers to read
 * \param dst A pointer to a byte array where the data should be stored. Must be large enough to hold len bytes.
 * \return I2C_SUCCESS If successful. Else either I2C_ERROR_WRITE_FAILURE or I2C_ERROR_READ_FAILURE.
 */
int i2c_bus_read_bytes(i2c_inst_t * bus, dev_addr dev, reg_addr reg, uint8_t reg_addr_len, uint len, byte * dst);

/**
 * \brief Write bytes to a register on a device connected to a I2C bus.
 * 
 * \param bus The I2C instance the device is attached to.
 * \param dev The device address. must be less than 0x80.
 * \param reg The register address on the device.
 * \param reg_addr_len The length of the address in bytes. Allows up to 4 byte (32bit) address spaces
 * \param len The number of registers to read
 * \param src A pointer to a byte array containing the bytes to write.
 * \return I2C_SUCCESS If successful. Else either I2C_ERROR_WRITE_FAILURE.
 */
int i2c_bus_write_bytes(i2c_inst_t * bus, dev_addr dev, reg_addr reg, uint8_t reg_addr_len, uint len, byte * src);

/*
 * \brief Read a range of bits from a register on an I2C device.
 * 
 * \param bus The I2C instance the device is attached to.
 * \param dev The device address. must be less than 0x80.
 * \param reg The register address on the device.
 * \param reg_addr_len The length of the address in bytes. Allows up to 4 byte (32bit) address spaces
 * \param from_bit The bit in the register to start reading from (inclusive)
 * \param to_bit The bit in the register to end reading (inclusive)
 * \param dst A pointer to a byte where the data will be stored.
 * 
 * \return I2C_SUCCESS If successful. Else either I2C_ERROR_WRITE_FAILURE or I2C_ERROR_READ_FAILURE.
 */
int i2c_bus_read_bits(i2c_inst_t * bus, dev_addr dev, reg_addr reg, uint8_t reg_addr_len, uint8_t from_bit, uint8_t to_bit, byte * dst);

/*
 * \brief Write to a range of bits on a register in an I2C device.
 * 
 * \param bus The I2C instance the device is attached to.
 * \param dev The device address. must be less than 0x80.
 * \param reg The register address on the device.
 * \param reg_addr_len The length of the address in bytes. Allows up to 4 byte (32bit) address spaces
 * \param from_bit The bit in the register to write from (inclusive)
 * \param to_bit The bit in the register to write to (inclusive)
 * \param dst The bit data that will be written to range. Bit 0 or val will be written to bit from_bit in reg.
 * 
 * \return I2C_SUCCESS If successful. Else either I2C_ERROR_WRITE_FAILURE or I2C_ERROR_READ_FAILURE.
 */
int i2c_bus_write_bits(i2c_inst_t * bus, dev_addr dev, reg_addr reg, uint8_t reg_addr_len, uint8_t from_bit, uint8_t to_bit, byte val);
#endif