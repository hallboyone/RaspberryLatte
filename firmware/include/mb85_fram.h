
/** @defgroup mb85_fram MB85 FRAM Library
 * 
 * \brief Driver for the MB85 series of FRAM ICs.
 * 
 * The MB85 series of FRAM ICs provide fast, non-volatile memory
 * in a variety of sizes. This library abstracts the process of saving variables remotely
 * on the FRAM over an I2C bus. Once the device has been setup using ::mb85_fram_setup, local 
 * memory locations can linked with ::mb85_fram_link_var. The local variable can be used
 * as normal in your program, and its value synced with the remote value on the FRAM IC
 * with ::mb85_fram_load and ::mb85_fram_save. 
 * 
 * @{
 * 
 * \file mb85_fram.h
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief MB85 FRAM header
 * \version 0.1
 * \date 2022-11-14
 */

#ifndef MB85_FRAM_H
#define MB85_FRAM_H

#include "pico/stdlib.h"
#include "i2c_bus.h"

/** \brief Indicate how the values should be initalized when linking remote var. */
typedef enum {
    MB85_FRAM_INIT_FROM_VAR = 0, /**< Set the FRAM var to the local var when linking */
    MB85_FRAM_INIT_FROM_FRAM = 1 /**< Set the local var to the FRAM var when linking */
    } mb85_fram_init_dir;

/** \brief Struct used internally to manage links with remote vars. */
typedef struct {
    uint8_t * local_addr;  /**< Pointer to local memory address holding local copy*/
    reg_addr remote_addr;  /**< Starting address of variable copy */
    uint16_t num_bytes;    /**< Number of bytes in memory object */
} mb85_fram_remote_var;

/** \brief Struct representing a single FRAM IC. */
typedef struct {
    i2c_inst_t * bus;            /**< I2C bus the chip is attached to */
    dev_addr addr;               /**< Value of address pins on MB85 device */
    mb85_fram_remote_var * vars; /**< Array of all remote vars linked to device */
    uint16_t num_vars;           /**< Number of vars linked to device */
    uint16_t var_buf_len;        /**< Size of vars buffer */
} mb85_fram;

/** \brief Verifies and configures a MB85 device struct. 
 * 
 * Once configured, the struct can be used to link local variables with remote variables stored in FRAM.
 * 
 * \param dev MB85 device struct that will be set up. 
 * \param nau7802_i2c Pointer to desired I2C instance. Should be initalized with i2c_bus_setup.
 * \param address_pins Bitfield indicating which of the three device address pins have been pulled high [0,7].
 * \param init_val Optional pointer to a value that will be used to overwrite the memory.
 * 
 * \return PICO_ERROR_NONE if no error is encountered.
 * \return PICO_ERROR_INVALID_ARG if address_pins are out of range (greater than 7).
 * \return PICO_ERROR_IO if physical device is not connected.
*/
int mb85_fram_setup(mb85_fram * dev, i2c_inst_t * nau7802_i2c, dev_addr address_pins, uint8_t * init_val);

/** \brief Finds the device's max address.
 * 
 * \param dev Initalized MB85 device struct to examine
 * 
 * \returns The max address of the MB85 device. 0 on error.
 */
uint16_t mb85_fram_get_max_addr(mb85_fram * dev);

/** \brief Set all memory locations in MB85 to the passed in value.
 * 
 * \param dev Initalized MB85 device to write to.
 * \param value The value to write to all memory locations.
 * 
 * \returns PICO_ERROR_NONE if successful. Else returns PICO_ERROR_IO
 */
int mb85_fram_set_all(mb85_fram * dev, uint8_t value);

/** \brief Creates an link between the local object pointed at by var and the 
 * memory location on the remote device.
 * 
 * \param dev Initalized MB85 device to link with.
 * \param var Pointer to the starting memory location of the local object that will be linked.
 * \param remote_addr Memory address where the remote variable is located.
 * \param num_bytes Size of object that will be stored. 
 * \param init_from_fram Indicates if the local variable is initalized from fram or vice vera (MB85_FRAM_INIT_FROM_VAR or MB85_FRAM_INIT_FROM_FRAM)
 * 
 * \returns PICO_ERROR_NONE on success. I2C Bus error is problem communicating over I2C. 
*/
int mb85_fram_link_var(mb85_fram * dev, void * var, reg_addr remote_addr, uint16_t num_bytes, mb85_fram_init_dir init_from_fram);

/** \brief Breaks the link if it exists between the local object pointed at by var and the 
 * memory location on the remote device.
 * 
 * \param dev Initalized MB85 device to link with.
 * \param var Pointer to the starting memory location of the local object that will be unlinked.
 * 
 * \returns PICO_ERROR_NONE. 
*/
int mb85_fram_unlink_var(mb85_fram * dev, void * var);

/** \brief Reads the current value stored in MB85 chip into remote_var 
 * 
 * \param dev Initalized MB85 device to read from.
 * \param var pointer to local variable linked to a remote variable
 * 
 * \returns PICO_ERROR_NONE on success. PICO_ERROR_INVALID_ARG if no link found. I2C Bus error is problem reading over I2C.  
*/
int mb85_fram_load(mb85_fram * dev, void * var);

/** \brief Writes the current values in remote_var onto MB85 chip 
 * 
 * \param dev Initalized MB85 device to wrtie to.
 * \param var pointer to local variable linked to a remote variable
 * 
 * \returns PICO_ERROR_NONE on success. PICO_ERROR_INVALID_ARG if no link found. I2C Bus error is problem writing over I2C.  
*/
int mb85_fram_save(mb85_fram * dev, void * var);
#endif
/** @} */