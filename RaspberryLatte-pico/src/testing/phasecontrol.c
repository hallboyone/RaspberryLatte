/**
 * Reads a duty cycle values from 0 to 100 over i2c and switches an ssr 
 * accordingly relative to zerocross times. Designed to provide adjustment
 * to ac-driven inductive loads where simple pwm with and ssr would result
 * in inductive voltage spikes
 */

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "pico/time.h"

#define TRIGGER 0x08 // 0x08 for RISING, 0x04 for FALLING

#define PIN_I2C_SDA   2
#define PIN_I2C_CLK   3
#define PIN_LED       PICO_DEFAULT_LED_PIN
#define PIN_ZEROCROSS 16
#define PIN_SSR       0
#define PIN_AC_ACTIVE 14

#define PERIOD_1_75_US     29167
#define PERIOD_1_00_US     16667
#define ZEROCROSS_DELAY_US 1620

volatile uint8_t zerocross_flag = 0;
volatile uint64_t zerocross_time = 0;

uint8_t duty_cycle = 0; // should range from 0-100
uint16_t timeout_val_us = 0;

const uint16_t timeouts_us[101] =
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

int64_t stop(int32_t alarm_num, void * data){
  gpio_put(PIN_SSR, 0);
  return 0;
}

int64_t start(int32_t alarm_num, void * data){
  gpio_put(PIN_SSR, 1);
  return 0;
}

void switch_scheduler(uint gpio, uint32_t events){
  zerocross_flag = 1;
  zerocross_time = time_us_64() - ZEROCROSS_DELAY_US;
}

void setup() {
  stdio_init_all();

  // Communicate over I2C
  i2c_set_slave_mode(i2c1, true, 0x60);
  gpio_set_function(PIN_I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(PIN_I2C_CLK, GPIO_FUNC_I2C);
  gpio_pull_up(PIN_I2C_SDA);
  gpio_pull_up(PIN_I2C_CLK);

  // Setup SSR output pin
  gpio_init(PIN_SSR);
  gpio_set_dir(PIN_SSR, GPIO_OUT);

  // Setup AC-active output pin
  gpio_init(PIN_AC_ACTIVE);
  gpio_set_dir(PIN_AC_ACTIVE, GPIO_OUT);

  // LED PWM output to indicate current duty_cycle
  gpio_set_function(PIN_LED, GPIO_FUNC_PWM);
  uint slice_num = pwm_gpio_to_slice_num(PIN_LED);
  pwm_set_enabled(slice_num, true);
  pwm_set_wrap(slice_num, 100);

  // Setup zero-cross input pin
  gpio_init(PIN_ZEROCROSS);
  gpio_set_dir(PIN_ZEROCROSS, GPIO_IN);
  gpio_set_pulls(PIN_ZEROCROSS, false, true);
  gpio_set_irq_enabled_with_callback(PIN_ZEROCROSS,
				     TRIGGER, true,
				     &switch_scheduler);
  return;
}

void main() {
  setup();
  while (true) {
    // If data is avalible, read into duty_cycle and update values
    if (i2c_get_read_available(i2c1)){
      i2c_read_raw_blocking(i2c1, &duty_cycle, 1);
      pwm_set_gpio_level(PIN_LED, duty_cycle);
      timeout_val_us = timeouts_us[100-duty_cycle];
    }

    // Signal if AC line is hot
    if (time_us_64() - zerocross_time > PERIOD_1_75_US){
      gpio_put(PIN_AC_ACTIVE, 0);
    }
    else {
      gpio_put(PIN_AC_ACTIVE, 1);
    }

    // If we crossed 0, schedule the next two switches
    if (zerocross_flag){
      zerocross_flag = 0;
      add_alarm_at(zerocross_time + PERIOD_1_75_US, &stop, NULL, false);
      if (duty_cycle > 0){
	add_alarm_at(zerocross_time + PERIOD_1_00_US + timeout_val_us, &start, NULL, false);
      }
    }
  }
}
