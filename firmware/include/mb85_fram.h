#ifndef MB85_FRAM_H
#define MB85_FRAM_H

#include "pico/stdlib.h"
#include "i2c_bus.h"

typedef struct {
    i2c_inst_t * bus;  /**< I2C bus the chip is attached to */
    dev_addr addr;     /**< Value of address pins on MB85 device */
} mb85_fram;

typedef struct {
    mb85_fram * dev;     /**< MB85 device holding remote var */
    reg_addr mem_addr;       /**< Starting address of variable memory */
    uint8_t * val;       /**< Pointer to allocated memory to store value in */
    uint16_t num_bytes;  /**< Number of bytes in memory object */
} remote_var;

/** \brief Check for a MB85 device on the indicated I2C bus at the device address
 * specified by the address_pins. If the device exists and init_val != NULL, init_val
 * is written to every memory block in device.
 * \param dev Structure for the MB85 device that will be set up. 
 * \param nau7802_i2c Pointer to desired I2C instance. Should be initalized with i2c_bus_setup.
 * \param address_pins Bitfield indicating which of the three device address pins have been pulled high.
 * \param init_val Optional pointer to a value that will be used to overwrite the memory.
*/
int mb85_fram_setup(mb85_fram * dev, i2c_inst_t * nau7802_i2c, dev_addr address_pins, uint8_t * init_val);

/** \brief Finds the device's memory capacity */
uint16_t mb85_fram_get_size(mb85_fram * dev);

/** \brief Set all memory locations in MB85 to the passed in value.
 * \param device A value between 0 and 7 matching the address pins of the chip to write to.
 * \param value The value to write to all memory locations.
 */
int mb85_fram_set_all(mb85_fram * dev, uint8_t value);

/** \brief Allocates memory for the memory object and fills it with whatever was on the
 * MB85 at that address.
 * \param var A remote_var object that will be initalized.
 * \param addr Starting address of variable memory.
 * \param num_bytes Size of object that will be stored. 
*/
int mb85_fram_init_var(mb85_fram * dev, remote_var * var, reg_addr addr, uint16_t num_bytes);

/** \brief Reads the current value stored in MB85 chip into remote_var */
int mb85_fram_read(remote_var * var);

/** \brief Writes the current values in remote_var onto MB85 chip */
int mb85_fram_write(remote_var * var);

#endif