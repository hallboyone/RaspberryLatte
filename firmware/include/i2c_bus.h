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

#define I2C_BUS_SUCCESS 0              /**< Code used to indicate a successfull I2C operations */
#define I2C_BUS_ERROR_WRITE_FAILURE -1 /**< Code used to indicate a failed I2C write operation */
#define I2C_BUS_ERROR_READ_FAILURE  -2 /**< Code used to indicate a failed I2C read operation */
#define I2C_BUS_ERROR_CONFIGURATION -3 /**< Code for invalid I2C configuration values */

typedef uint8_t byte;           /**< Type name for a single byte of data */
typedef uint8_t dev_addr;       /**< Type name for a device address */
typedef uint32_t reg_addr;      /**< Type name for up to a 32 bit register address. */

/**
 * \brief Struct that defines a range of bits in a byte address.
 */
typedef struct{
    uint8_t from;         /**< The LSB in the bit range (inclusive). */
    uint8_t to;           /**< The MSB in the bit range (inclusive). */
    reg_addr in_reg;      /**< Address of register containing bit range */
    uint8_t reg_addr_len; /**< Address space for I2C device */
} bit_range;

/**
 * \brief Setup a I2C bus attached to the indicated pins.
 * 
 * \param bus Pointer to i2c_inst_t that will be initalized
 * \param baudrate The target baudrate.
 * \param scl_pin GPIO number for the clock pin
 * \param sda_pin GPIO number for the data pin
 * 
 * \return I2C_BUS_SUCCESS If successful and I2C_BUS_ERROR_CONFIGURATION if configutation failed.
 */
int i2c_bus_setup(i2c_inst_t * bus, uint baudrate, uint8_t scl_pin, uint8_t sda_pin);

/**
 * \brief Checks to see if a device at the given address is attached to i2c bus
 * 
 * \param bus Pointer to i2c_inst_t where the device will be looked for
 * \param dev Address of device to search for.
 */
bool i2c_bus_is_connected(i2c_inst_t * bus, dev_addr dev);

/**
 * \brief Sets the bits within buf specified by the from_bit and to_bit. Only the specified bits
 * are ever written to. If the value does not fit within the specified bits, the highest 
 * bits are truncated.
 * 
 * \param buf Pointer to byte that will be updated
 * \param bits The range of bits to set.
 * \param val The value to pack into byte. Bit 0 is set to the from_bit and so on.
 */
void i2c_bus_set_bits(byte* buf, bit_range bits, uint8_t val);

/**
 * \brief Read bytes from a register on a device connected to a I2C bus.
 * 
 * \param bus The I2C instance the device is attached to.
 * \param dev The device address. must be less than 0x80.
 * \param reg The register address on the device.
 * \param reg_addr_len The length of the address in bytes. Allows up to 4 byte (32bit) address spaces
 * \param len The number of registers to read
 * \param dst A pointer to a byte array where the data should be stored. Must be large enough to hold len bytes.
 * \return I2C_BUS_SUCCESS If successful. Else either I2C_BUS_ERROR_WRITE_FAILURE or I2C_BUS_ERROR_READ_FAILURE.
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
 * \return I2C_BUS_SUCCESS If successful. Else either I2C_BUS_ERROR_WRITE_FAILURE.
 */
int i2c_bus_write_bytes(i2c_inst_t * bus, dev_addr dev, reg_addr reg, uint8_t reg_addr_len, uint len, byte * src);

/*
 * \brief Read a range of bits from a register on an I2C device.
 * 
 * \param bus The I2C instance the device is attached to.
 * \param dev The device address. must be less than 0x80.
 * \param bits The range of bits to read from.
 * \param dst A pointer to a byte where the data will be stored.
 * 
 * \return I2C_BUS_SUCCESS If successful. Else either I2C_BUS_ERROR_WRITE_FAILURE or I2C_BUS_ERROR_READ_FAILURE.
 */
int i2c_bus_read_bits(i2c_inst_t * bus, const dev_addr dev, const bit_range bits, byte * dst);

/*
 * \brief Write to a range of bits on a register in an I2C device.
 * 
 * \param bus The I2C instance the device is attached to.
 * \param dev The device address. must be less than 0x80.
 * \param bits The range of bits to write to.
 * \param val The bit data that will be written to range. Bit 0 of val will be written to bit from_bit in reg.
 * 
 * \return I2C_BUS_SUCCESS If successful. Else either I2C_BUS_ERROR_WRITE_FAILURE or I2C_BUS_ERROR_READ_FAILURE.
 */
int i2c_bus_write_bits(i2c_inst_t * bus, const dev_addr dev, const bit_range bits, const byte val);
#endif