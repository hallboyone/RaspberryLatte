/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"

#define RISING 8
#define FALLING 4

const uint16_t half_period_us = 8333;
const uint16_t quarter_period_us = 4166;
const uint16_t opto_sig_delay = 36;

const uint8_t level = 100;
const uint16_t value = 0;

const uint16_t timeouts_usec[101] =
  {0, 531, 753, 924, 1068, 1196, 1313, 1421, 1521, 1616,
   1707, 1793, 1877, 1957, 2035, 2110, 2183, 2255, 2324,
   2393, 2460, 2525, 2590, 2654, 2716, 2778, 2839, 2899,
   2958, 3017, 3075, 3133, 3190, 3246, 3303, 3358, 3414,
   3469, 3524, 3578, 3633, 3687, 3740, 3794, 3848, 3901,
   3954, 4007, 4061, 4114, 4167, 4220, 4273, 4326, 4379,
   4432, 4486, 4539, 4593, 4647, 4701, 4755, 4810, 4864,
   4919, 4975, 5031, 5087, 5144, 5201, 5258, 5316, 5375,
   5435, 5495, 5556, 5617, 5680, 5743, 5808, 5874, 5941,
   6009, 6079, 6150, 6223, 6299, 6376, 6457, 6540, 6626,
   6717, 6812, 6913, 7020, 7137, 7265, 7410, 7581, 7802,
   8333};

const uint8_t opto_sig_pin = 5;
const uint8_t control_sig_pin = 25; //PICO_DEFAULT_LED_PIN;
const uint8_t start_alarm_num = 0;
const uint8_t stop_alarm_num = 1;

void stop(uint alarm_num){
  /** Turn off the control signal and cancel any start target **/
  gpio_put(control_sig_pin, 0);
  hardware_alarm_cancel(start_alarm_num);
}

void start(uint alarm_num){
  gpio_put(control_sig_pin, 1);
}

void delayed_action(uint gpio, uint32_t events){
  if (events & RISING){
    if (level==100){
      start(0);
    }
    else if(level==0){
      stop(0);
    }
    else{
      // Set the start_alarm_num for some small amount of time in the future
      hardware_alarm_set_target(start_alarm_num, time_us_64() + value - opto_sig_delay);
    }
  }
  else if (events & FALLING){
    // Set the stop_alarm_num for a quarter cycle in the future
    hardware_alarm_set_target(stop_alarm_num, time_us_64() + quarter_period_us - opto_sig_delay);
  }
}

void setup() {
  gpio_init(control_sig_pin);
  gpio_init(opto_sig_pin);
  gpio_set_dir(control_sig_pin, GPIO_OUT);
  gpio_set_dir(opto_sig_pin, GPIO_IN);
  gpio_set_pulls(opto_sig_pin, false, true);

  hardware_alarm_set_callback(start_alarm_num, &start);
  hardware_alarm_set_callback(stop_alarm_num, &stop);

  gpio_set_irq_enabled(opto_sig_pin, FALLING | RISING, true);
  gpio_set_irq_enabled_with_callback(opto_sig_pin, FALLING | RISING, 1, &delayed_action);
}

int main() {
  setup();
  gpio_init(6);
  gpio_set_dir(6, GPIO_OUT);
  while (true) {
    gpio_put(6, 1);
    sleep_ms(1000);
    gpio_put(6, 0);
    sleep_ms(1000);
  }
  return 0;
}
