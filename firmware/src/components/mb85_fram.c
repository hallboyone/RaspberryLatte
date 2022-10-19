#include "mb85_fram.h"
#include <string.h>
#include <stdlib.h>

#define MB85_DEVICE_CODE 0b1010000

void static mb85_fram_i2c_read(mb85_fram * dev, reg_addr mem_addr, uint16_t len, uint8_t * dst){
    i2c_bus_read_bytes(dev->bus, dev->addr, mem_addr, 2, len, dst);
}

void static mb85_fram_i2c_write(mb85_fram * dev, reg_addr mem_addr, uint16_t len, uint8_t * src){
    i2c_bus_write_bytes(dev->bus, dev->addr, mem_addr, 2, len, src);
}

/** \brief Check for a MB85 device on the indicated I2C bus at the device address
 * specified by the address_pins. If the device exists and init_val != NULL, init_val
 * is written to every memory block in device.
 * \param dev Structure for the MB85 device that will be set up. 
 * \param nau7802_i2c Pointer to desired I2C instance. Should be initalized with i2c_bus_setup.
 * \param address_pins Bitfield indicating which of the three device address pins have been pulled high.
 * \param init_val Optional pointer to a value that will be used to overwrite the memory.
*/
int mb85_fram_setup(mb85_fram * dev, i2c_inst_t * nau7802_i2c, dev_addr address_pins, uint8_t * init_val){
    if (address_pins > 7) return PICO_ERROR_INVALID_ARG;
    dev->addr = MB85_DEVICE_CODE & address_pins;
    dev->bus = nau7802_i2c;
    if(!i2c_bus_is_connected(dev->bus, dev->addr)) return PICO_ERROR_IO;

    if(init_val != NULL){
        mb85_fram_set_all(dev, *init_val);
    }

    return PICO_ERROR_NONE;
}

/** \brief Finds the device's memory capacity */
uint16_t mb85_fram_get_size(mb85_fram * dev){
    uint8_t byte_0 = 0;
    mb85_fram_i2c_read(dev, 0, 1, &byte_0);
    uint16_t mem_len = 1;
    uint8_t byte_buf;
    while(true){
        mb85_fram_i2c_read(dev, mem_len, 1, &byte_buf);
        if (byte_buf == byte_0){
            byte_buf += 1;
            mb85_fram_i2c_write(dev, mem_len, 1, &byte_buf); // Write to what may be 0
            mb85_fram_i2c_read(dev, 0, 1, &byte_buf);        // Read from 0 byte
            mb85_fram_i2c_write(dev, mem_len, 1, &byte_0); // Write original value back
            if (byte_buf != byte_0){ 
                // If write at mem_len changed byte 0 (addresses have wrapped)
                return mem_len;
            }
        }
        mem_len += 1;
    }
}

/** \brief Set all memory locations in MB85 to the passed in value.
 * \param device A value between 0 and 7 matching the address pins of the chip to write to.
 * \param value The value to write to all memory locations.
 */
int mb85_fram_set_all(mb85_fram * dev, uint8_t value){
    uint8_t repeated_value [32];
    memset(repeated_value, value, 32);

    // Set byte 0 to something other than the new value
    uint8_t byte_buf = value + 1;
    mb85_fram_i2c_write(dev, 0, 1, &byte_buf);

    uint16_t mem_addr = 1;
    do {
        mb85_fram_i2c_write(dev, mem_addr, 32, repeated_value);
        mb85_fram_i2c_read(dev, 0, 1, &byte_buf);
    } while (byte_buf != value);
}

/** \brief Allocates memory for the memory object and fills it with whatever was on the
 * MB85 at that address.
 * \param var A remote_var object that will be initalized.
 * \param addr Starting address of variable memory.
 * \param num_bytes Size of object that will be stored. 
*/
int mb85_fram_init_var(mb85_fram * dev, remote_var * var, reg_addr mem_addr, uint16_t num_bytes){
    var->dev = dev;
    var->mem_addr = mem_addr;
    var->num_bytes = num_bytes;
    var->val = malloc(num_bytes);
    
    mb85_fram_read(var);
}

/** \brief Reads the current value stored in MB85 chip into remote_var */
int mb85_fram_read(remote_var * var){
    mb85_fram_i2c_read(var->dev, var->mem_addr, var->num_bytes, var->val);
}

/** \brief Writes the current values in remote_var onto MB85 chip */
int mb85_fram_write(remote_var * var){
    mb85_fram_i2c_write(var->dev, var->mem_addr, var->num_bytes, var->val);
}