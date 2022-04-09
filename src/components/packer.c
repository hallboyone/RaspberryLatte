#include "packer.h"

void packData(uint64_t scale_val, uint64_t thermo_val, uint64_t switch_vals, uint64_t pressure_val, uint64_t * buf){
  *buf = 0;
  // 0-15
  *buf = ((thermo_val & 0x0000ff00)>>8) | ((thermo_val & 0x000000ff)<<8);
  // 16-31
  *buf = (*buf) | ((pressure_val & 0xff00) << 8) | ((pressure_val & 0x00ff) << 24);
  // 32-34
  *buf = (*buf) | switch_vals<<32;
  //scale_val = 3;
  //scale_val <<= 30;
  //thermo_val = 0; 
  //*buf = (((uint64_t)switch_vals)<<(24+14)) | (((uint64_t)thermo_val)) | ((uint64_t)(scale_val));
}