
#include "pico/stdlib.h"

/** 
 * \brief Marks a parameter as unused in the function body to suppress warnings 
 * 
 * From an answer here: https://stackoverflow.com/questions/3599160/how-can-i-suppress-unused-parameter-warnings-in-c
 */
#define UNUSED_PARAMETER(x) (void)(x)


/**
 * \brief Clips value at bounds
*/
#define CLAMP(x, low, high) MIN(MAX(x,low),high)