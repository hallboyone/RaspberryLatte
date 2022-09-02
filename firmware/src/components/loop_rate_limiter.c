#include "loop_rate_limiter.h"

void loop_rate_limiter_us(uint64_t * last_loop_time_us, const uint64_t loop_period_us){
    if(*last_loop_time_us==0){
        *last_loop_time_us = time_us_64();
    }
    else{
        while(*last_loop_time_us + loop_period_us > time_us_64()){
            tight_loop_contents();
        }
        *last_loop_time_us = time_us_64();
    }
}