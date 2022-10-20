#include "mb85_fram.h"
#include <string.h>
#include <stdlib.h>

#define MB85_DEVICE_CODE 0b1010000

/** \brief Read value from MB85 device over I2C 
 * 
 * \param dev Initalized MB85 device to read from.
 * \param mem_addr Starting memory address to read from.
 * \param len Number of bytes to read.
 * \param dst Pointer to preallocated array where the data will be stored.
 * 
 * \returns 0 on success. I2C_BUS error code on failure.
*/
static int mb85_fram_i2c_read(mb85_fram * dev, reg_addr mem_addr, uint16_t len, uint8_t * dst){
    return i2c_bus_read_bytes(dev->bus, dev->addr, mem_addr, 2, len, dst);
}

/** \brief Write value to MB85 device over I2C
 * 
 * \param dev Initalized MB85 device to write to.
 * \param mem_addr Starting memory address to write to.
 * \param len Number of bytes to write.
 * \param dst Pointer to data array of length \p len holding the data to be written.
 * 
 * \returns 0 on success. I2C_BUS error code on failure.
*/
static int mb85_fram_i2c_write(mb85_fram * dev, reg_addr mem_addr, uint16_t len, uint8_t * src){
    return i2c_bus_write_bytes(dev->bus, dev->addr, mem_addr, 2, len, src);
}

/** \brief Iterate through the current device variables looking for one matching var.
 * 
 * \param dev Initalized MB85 device struct that will be searched.
 * \param var Pointer to variable is the subject of the search.
 * 
 * \returns Pointer to internal variable structure if a match is found. Else, returns NULL.
*/
static mb85_fram_remote_var * mb85_fram_find_var(mb85_fram * dev, void * var){
    uint8_t * var_addr = (uint8_t*)var;
    uint16_t var_idx = 0;
    for(var_idx = 0; var_idx < dev->num_vars; var_idx++){
        if (dev->vars[var_idx].local_addr == var_addr) return &(dev->vars[var_idx]);
    }
    return NULL;
}

/** \brief Double the size of the variable buffer if full. 
 * 
 * \param dev Initalized MB85 device struct.
*/
static void mb85_fram_resize_buf(mb85_fram * dev){
    if(dev->var_buf_len == dev->num_vars){
        mb85_fram_remote_var * old_var_buf = dev->vars;
        dev->vars = (mb85_fram_remote_var*)malloc(2*dev->var_buf_len*sizeof(mb85_fram_remote_var));
        memcmp(dev->vars, old_var_buf, dev->num_vars*sizeof(mb85_fram_remote_var));
        dev->var_buf_len *= 2;
    }
}

int mb85_fram_setup(mb85_fram * dev, i2c_inst_t * nau7802_i2c, dev_addr address_pins, uint8_t * init_val){
    if (address_pins > 7) return PICO_ERROR_INVALID_ARG;
    dev->addr = MB85_DEVICE_CODE | address_pins;
    dev->bus = nau7802_i2c;
    if(!i2c_bus_is_connected(dev->bus, dev->addr)) return PICO_ERROR_IO;

    dev->var_buf_len = 16;
    dev->vars = (mb85_fram_remote_var*)malloc(dev->var_buf_len*sizeof(mb85_fram_remote_var));
    dev->num_vars = 0;
    
    if(init_val != NULL){
        mb85_fram_set_all(dev, *init_val);
    }

    return PICO_ERROR_NONE;
}

uint16_t mb85_fram_get_size(mb85_fram * dev){
    uint8_t byte_0 = 0;
    if(mb85_fram_i2c_read(dev, 0, 1, &byte_0)) return 0;
    uint16_t mem_len = 1;
    uint8_t byte_buf;
    while(true){
        mb85_fram_i2c_read(dev, mem_len, 1, &byte_buf);
        if (byte_buf == byte_0){
            byte_buf += 1;
            if(mb85_fram_i2c_write(dev, mem_len, 1, &byte_buf)) return 0; // Write to what may be 0
            if(mb85_fram_i2c_read(dev, 0, 1, &byte_buf)) return 0;        // Read from 0 byte
            if(mb85_fram_i2c_write(dev, mem_len, 1, &byte_0)) return 0; // Write original value back
            if (byte_buf != byte_0){ 
                // If write at mem_len changed byte 0 (addresses have wrapped)
                return mem_len;
            }
        }
        mem_len += 1;
    }
}

int mb85_fram_set_all(mb85_fram * dev, uint8_t value){
    // By setting the values 32 registers at a time, the runtime is dramatically reduced.
    uint8_t repeated_value [32];
    memset(repeated_value, value, 32);

    // Set byte 0 to something other than the new value
    uint8_t byte_buf = value + 1;
    if(mb85_fram_i2c_write(dev, 0, 1, &byte_buf)) return PICO_ERROR_IO;

    uint16_t mem_addr = 1;
    do {
        if(mb85_fram_i2c_write(dev, mem_addr, 32, repeated_value)) return PICO_ERROR_IO;
        if(mb85_fram_i2c_read(dev, 0, 1, &byte_buf)) return PICO_ERROR_IO;
    } while (byte_buf != value);

    return PICO_ERROR_NONE;
}

int mb85_fram_link_var(mb85_fram * dev, void * var, reg_addr remote_addr, uint16_t num_bytes, init_dir init_from_fram){
    mb85_fram_resize_buf(dev);
    dev->vars[dev->num_vars].local_addr = (uint8_t*)var;
    dev->vars[dev->num_vars].remote_addr = remote_addr;
    dev->vars[dev->num_vars].num_bytes = num_bytes;

    dev->num_vars += 1;

    if(init_from_fram){
        return mb85_fram_read(dev, var);
    } else {
        return mb85_fram_write(dev, var);
    }
}

int mb85_fram_read(mb85_fram * dev, void * var){
    mb85_fram_remote_var * var_s = mb85_fram_find_var(dev, var);
    if(var_s == NULL) return PICO_ERROR_INVALID_ARG;
    return mb85_fram_i2c_read(dev, var_s->remote_addr, var_s->num_bytes, (uint8_t*)var);
}

int mb85_fram_write(mb85_fram * dev, void * var){
    mb85_fram_remote_var * var_s = mb85_fram_find_var(dev, var);
    if(var_s == NULL) return PICO_ERROR_INVALID_ARG;
    return mb85_fram_i2c_write(dev, var_s->remote_addr, var_s->num_bytes, (uint8_t*)var);
}