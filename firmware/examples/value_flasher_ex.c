/**
 * \ingroup value_flasher
 * @{
 * 
 * \file value_flasher_ex.c
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Example usage of value flasher
 * \version 0.1
 * \date 2022-11-14
 * 
 * Display the value 132 using print statements and a value_flasher object. The output should repeat
 * the pattern
 * 
 * % ooo\n
 * % xoo\n
 * % ooo\n
 * % oxo\n
 * % ooo\n
 * % oxo\n
 * % ooo\n
 * % oxo\n
 * % ooo\n
 * % oox\n
 * % ooo\n
 * % oox\n
 * % ooo\n
 */
#include "value_flasher.h"
#include <stdio.h>

int main(){
    const uint VALUE = 132;
    const uint PERIOD_MS = 750;
    uint8_t bitfield = 0;

    value_flasher vf;
    value_flasher_setup(&vf, VALUE, PERIOD_MS, &bitfield);

    uint8_t old_bitfield = bitfield - 1;
    while(true){
        if(bitfield != old_bitfield){
            if(bitfield & 0b001)      printf("oox\n"); // Print 1's blink
            else if(bitfield & 0b010) printf("oxo\n"); // Print 2's blink
            else if(bitfield & 0b100) printf("xoo\n"); // Print 3's blink
            else                      printf("ooo\n"); // Print all off
            old_bitfield = bitfield;
        }
    }
}
/** @} */