/**
 * Interface for a SPST and SP4T switch. The state is encoded in a 8 bit value as
 *       [7-3: Not used][2: SPST off/on][1-0: SP4T value]
 */

#include "pico/stdlib.h"

/**
 * Structure holding the configuration values and latest state of the espresso 
 * machine's phyiscal inputs
 */
typedef struct {
  uint8_t gpio_pump;
  uint8_t gpio_dial[4];
  uint8_t state;
} PhysicalInputs;

/**
 * Initializes the physical input pins and reads their current value
 *
 * @param s Pointer to PhysicalInputs structure with configuration values. 
 * The input state is also stored in this structure.
 */
void physical_inputs_setup(PhysicalInputs * s);

/**
 * Read the physical inputs and store their values in the given structure.
 *
 * @param s Pointer to PhysicalInputs structure with configuration values. 
 * The input state is also stored in this structure.
 */
void physical_inputs_read(PhysicalInputs * s);

/**
 * Returns the current state of the spst switch
 *
 * @param s Pointer to PhysicalInputs structure with configuration values. 
 * The input state is also stored in this structure.
 *
 * @return 1 if spst pin is high. 0 else.
 */
uint8_t physical_inputs_spst(PhysicalInputs * s);

/**
 * Returns the current state of the sp4t switch
 *
 * @param s Pointer to PhysicalInputs structure with configuration values. 
 * The input state is also stored in this structure.
 *
 * @return Value from 0-3 indicating which of the 4 throws is active.
 */
uint8_t physical_inputs_sp4t(PhysicalInputs * s);