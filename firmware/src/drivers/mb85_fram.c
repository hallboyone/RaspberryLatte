/**
 * \ingroup mb85_fram
 * @{
 * 
 * \file mb85_fram.c
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief MB85 FRAM source
 * \version 0.1
 * \date 2022-11-14
 */
#include "drivers/mb85_fram.h"
#include <string.h>
#include <stdlib.h>

#define MB85_DEVICE_CODE 0b1010000

/** \brief Struct used internally to manage links with remote vars. */
typedef struct {
    uint8_t * local_addr;  /**< Pointer to local memory address holding local copy*/
    reg_addr remote_addr;  /**< Starting address of variable copy */
    uint16_t num_bytes;    /**< Number of bytes in memory object */
} mb85_fram_remote_var;

/** \brief Struct representing a single FRAM IC. */
typedef struct mb85_fram_s {
    i2c_inst_t * bus;            /**< I2C bus the chip is attached to */
    dev_addr addr;               /**< Value of address pins on MB85 device */
    mb85_fram_remote_var * vars; /**< Array of all remote vars linked to device */
    uint16_t num_vars;           /**< Number of vars linked to device */
    uint16_t var_buf_len;        /**< Size of vars buffer */
} mb85_fram_;

/** \brief Read value from MB85 device over I2C 
 * 
 * \param dev Initalized MB85 device to read from.
 * \param mem_addr Starting memory address to read from.
 * \param len Number of bytes to read.
 * \param dst Pointer to preallocated array where the data will be stored.
 * 
 * \returns 0 on success. I2C_BUS error code on failure.
*/
static int mb85_fram_i2c_read(mb85_fram dev, reg_addr mem_addr, uint16_t len, uint8_t * dst){
    return i2c_bus_read_bytes(dev->bus, dev->addr, mem_addr, 2, len, dst);
}

/** \brief Write value to MB85 device over I2C
 * 
 * \param dev Initalized MB85 device to write to.
 * \param mem_addr Starting memory address to write to.
 * \param len Number of bytes to write.
 * \param src Pointer to data array of length \p len holding the data to be written.
 * 
 * \returns 0 on success. I2C_BUS error code on failure.
*/
static int mb85_fram_i2c_write(mb85_fram dev, reg_addr mem_addr, uint16_t len, uint8_t * src){
    return i2c_bus_write_bytes(dev->bus, dev->addr, mem_addr, 2, len, src);
}

/** \brief Iterate through the current device variables looking for one matching var.
 * 
 * \param dev Initalized MB85 device struct that will be searched.
 * \param var Pointer to variable is the subject of the search.
 * 
 * \returns Pointer to internal variable structure if a match is found. Else, returns NULL.
*/
static mb85_fram_remote_var * mb85_fram_find_var(mb85_fram dev, void * var){
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
static void mb85_fram_resize_buf(mb85_fram dev){
    if(dev->var_buf_len == dev->num_vars){
        mb85_fram_remote_var * old_var_buf = dev->vars;
        dev->vars = (mb85_fram_remote_var*)malloc(2*dev->var_buf_len*sizeof(mb85_fram_remote_var));
        memcpy(dev->vars, old_var_buf, dev->num_vars*sizeof(mb85_fram_remote_var));
        free(old_var_buf);
        dev->var_buf_len *= 2;
    }
}

mb85_fram mb85_fram_setup(i2c_inst_t * nau7802_i2c, dev_addr address_pins, uint8_t * init_val){
    if (address_pins > 7) return NULL;

    mb85_fram dev = malloc(sizeof(mb85_fram_));
    
    dev->addr = MB85_DEVICE_CODE | address_pins;
    dev->bus = nau7802_i2c;
    if(!i2c_bus_is_connected(dev->bus, dev->addr)) return NULL;

    dev->var_buf_len = 16;
    dev->vars = (mb85_fram_remote_var*)malloc(dev->var_buf_len*sizeof(mb85_fram_remote_var));
    dev->num_vars = 0;
    
    if(init_val != NULL){
        mb85_fram_set_all(dev, *init_val);
    }

    return dev;
}

uint16_t mb85_fram_get_max_addr(mb85_fram dev){
    uint32_t size_options [7] = {1<<9, 1<<11, 1<<13, 1<<14, 1<<15, 1<<16, 1<<17};

    // Save byte 0
    uint8_t byte_0 = 0;
    if(mb85_fram_i2c_read(dev, 0, 1, &byte_0)) return 0;

    uint8_t byte_buf;
    for (uint8_t i = 0; i<7; i++){
        if(mb85_fram_i2c_read(dev, size_options[i], 1, &byte_buf)) return 0;
        if(byte_buf==byte_0){
            // Write a different value and see if byte 0 changes.
            byte_buf += 1;
            if(mb85_fram_i2c_write(dev, size_options[i], 1, &byte_buf)) return 0;
            if(mb85_fram_i2c_read(dev, 0, 1, &byte_buf)) return 0;
            if(mb85_fram_i2c_write(dev, size_options[i], 1, &byte_0)) return 0;
            if (byte_buf != byte_0){
                const uint32_t max_addr = size_options[i] - 1;
                return max_addr;
            }
        }
    }
    return 0;
}

int mb85_fram_set_all(mb85_fram dev, uint8_t value){
    uint32_t cap = mb85_fram_get_max_addr(dev) + 1;

    // By setting the values 256 registers at a time, the runtime is dramatically reduced.
    const uint16_t write_size = 256;
    uint8_t repeated_value [write_size];
    memset(repeated_value, value, write_size);

    for(uint32_t i = 0; i < cap; i += write_size){
        mb85_fram_i2c_write(dev, i, write_size, repeated_value);
    }
    return PICO_ERROR_NONE;
}

int mb85_fram_link_var(mb85_fram dev, void * var, reg_addr remote_addr, uint16_t num_bytes, mb85_fram_init_dir init_from_fram){
    mb85_fram_resize_buf(dev);
    dev->vars[dev->num_vars].local_addr = (uint8_t*)var;
    dev->vars[dev->num_vars].remote_addr = remote_addr;
    dev->vars[dev->num_vars].num_bytes = num_bytes;

    dev->num_vars += 1;

    if(init_from_fram){
        return mb85_fram_load(dev, var);
    } else {
        return mb85_fram_save(dev, var);
    }
}

int mb85_fram_unlink_var(mb85_fram dev, void * var){
    for (uint16_t i = 0; i < dev->num_vars; i++){
        if (dev->vars[i].local_addr == var){
            // Shift all remaining elements left by 1 space to overwrite unlinked var
            for (uint16_t j = i+1; j < dev->num_vars; j++){
                dev->vars[j-1] = dev->vars[j];
            }
            dev->num_vars -= 1;
            return PICO_ERROR_NONE;
        }
    }
    return PICO_ERROR_NONE;
}

int mb85_fram_load(mb85_fram dev, void * var){
    mb85_fram_remote_var * var_s = mb85_fram_find_var(dev, var);
    if(var_s == NULL) return PICO_ERROR_INVALID_ARG;
    return mb85_fram_i2c_read(dev, var_s->remote_addr, var_s->num_bytes, (uint8_t*)var);
}

int mb85_fram_save(mb85_fram dev, void * var){
    mb85_fram_remote_var * var_s = mb85_fram_find_var(dev, var);
    if(var_s == NULL) return PICO_ERROR_INVALID_ARG;
    return mb85_fram_i2c_write(dev, var_s->remote_addr, var_s->num_bytes, (uint8_t*)var);
}

void mb85_fram_deinit(mb85_fram dev){
    free(dev->vars);
    free(dev);
}
/** @} */