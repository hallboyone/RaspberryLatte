#include "pid.h"

pid_data_t sample_sensor();

void sample_plant(float u);

int main(){
    pid_gains K = = {.p = 0.05, .i = 0.0015, .d = 0.0005};
    pid_ctrl ctrl;
    pid_setup(&ctrl, K, &sample_sensor, NULL, &sample_plant, 100, 0, 100, 500);

    while(true) pid_tick(&boiler_ctrl);
}