/**
 * \defgroup fixed_point Fixed point arithmetic utilities 
 * \ingroup utils
 * 
 * \brief Tools for fixed point arithmetic.
 * 
 * \{
 * 
 * \file fixed_point.h
 * \author Richard Hall (hallboyone@icloud.com)
 * \brief Fixed point library header
 * \version 0.1
 * \date 2023-03-07
*/
#ifndef FIXED_POINT_H
#define FIXED_POINT_H

#include "pico/stdlib.h"

/**
 * \brief Fixed point data type that is signed with 27 int bits and 4 frac bits.
 * 
 * Int range: +/- 134,217,728
 * Frac range: 0.0000 : 0.0625 : 0.9375
*/
typedef uint32_t fxp_s27x4;

#endif
/** \} */