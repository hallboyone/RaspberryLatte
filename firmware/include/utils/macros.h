
/** 
 * \brief Marks a parameter as unused in the function body to suppress warnings 
 * 
 * From an answer here: https://stackoverflow.com/questions/3599160/how-can-i-suppress-unused-parameter-warnings-in-c
 */
#define UNUSED_PARAMETER(x) (void)(x)