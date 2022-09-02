#include "pid.h"

float sensor_read_float();

void plant_apply_float(float u);

int main(){
    pid_ctrl boiler_ctrl = {.setpoint = 95, .K = {.p = 0.05, .i = 0.0015, .d = 0.0005}, 
                        .sensor = &sensor_read_float, .plant = &plant_apply_float,
                        .min_time_between_ticks_ms = 100};
    pid_init(&boiler_ctrl, 0, 100, 0);

    while(true) pid_tick(&boiler_ctrl);
}