/**
 * Peripherals register their maintenance functions and intervals using <registerMaintenance>.
 * Then, frequent polling of the <runMaintenance> program ensures that the correct functions
 * are called on the appropriate intervals. The faster the polling, the closer to the desired
 * intervals.
 */

#include "pico/stdlib.h"

typedef void (*Maintainer)();

/**
 * Save the pointer to the maintainer function along with its service interval in us
 */
void registerMaintainer(Maintainer fun);

/**
 * Iterate through all registered maintainers and call them if their service interval 
 * has expired.
 */
void runMaintenance();