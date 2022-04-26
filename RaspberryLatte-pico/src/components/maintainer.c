#include "maintainer.h"


static Maintainer _registered_maintainers [8] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
static uint8_t _num_maintainers = 0;

/**
 * Save the pointer to the maintainer function along with its service interval in us
 */
void registerMaintainer(Maintainer fun){
    _registered_maintainers[_num_maintainers] = fun;
    _num_maintainers += 1;
}

/**
 * Iterate through all registered maintainers and call them if their service interval 
 * has expired.
 */
void runMaintenance(){
    for(uint8_t n = 0; n < _num_maintainers; n++){
        _registered_maintainers[n]();
    }
}