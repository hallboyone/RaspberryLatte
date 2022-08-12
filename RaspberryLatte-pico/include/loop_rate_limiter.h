#include "pico/stdlib.h"

void loop_rate_limiter_us(uint64_t * last_loop_time_us, const uint64_t loop_period_us);
