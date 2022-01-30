/**
 * Reads a duty cycle values from 0 to 100 over i2c and switches an ssr 
 * accordingly relative to zerocross times. Designed to provide adjustment
 * to ac-driven inductive loads where simple pwm with and ssr would result
 * in inductive voltage spikes
 */

#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/multicore.h"

#define RISING             0x08
#define FALLING            0x04
#define PERIOD_1_25_US     20833
#define PERIOD_1_00_US     16667
#define PERIOD_0_50_US     8333

// Message instructions. Or'd over value if applicable
#define SET_DUTY           0x10000000
#define DUTY_MASK          0x0000008F
#define IS_AC_ON           0x20000000

typedef struct {
  uint8_t trigger;
  uint8_t zero_cross_pin;
  uint8_t out_pin;
  uint    zero_cross_delay;
} PHASECONTROL_CONFIG;

static uint64_t zerocross_time = 0;

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

static int64_t stop(int32_t alarm_num, void * data){
  uint8_t pin = *(uint8_t*)data;
  gpio_put(pin, 0);
  return 0;
}

static int64_t start(int32_t alarm_num, void * data){
  uint8_t pin = *(uint8_t*)data;
  gpio_put(pin, 1);
  return 0;
}

static void switch_scheduler(uint gpio, uint32_t events){
  zerocross_time = time_us_64();
}

/**
 * The main loop running on core1. After getting the configuration values from
 * core0, it sets up the pins and the zerocross interrupt and enters an infinite
 * loop. This loop check for a new duty cycle and data requests from core0 and 
 * schedules switching times if a zerocross was detected.
 */
static void phasecontrol_loop() {
  // Get packed config values from core 0
  uint32_t config_vals = multicore_fifo_pop_blocking();
  uint8_t out_pin = config_vals & 255;
  uint8_t zerocross_pin = (config_vals>>8) & 255;
  uint8_t trigger = (config_vals>>16) & 255;

  // Get zerocross time from core 0
  uint32_t zerocross_delay = multicore_fifo_pop_blocking();
  
  // Setup SSR output pin
  gpio_init(out_pin);
  gpio_set_dir(out_pin, GPIO_OUT);

  // Setup zero-cross input pin
  gpio_init(zerocross_pin);
  gpio_set_dir(zerocross_pin, GPIO_IN);
  gpio_set_pulls(zerocross_pin, false, true);
  gpio_set_irq_enabled_with_callback(zerocross_pin,
				     trigger, true,
				     &switch_scheduler);
  
  uint32_t timeout_us = 0;
  uint64_t prev_zerocross_time = 0;
  while (true) {
    // If data is avalible, read into duty_cycle and update values
    while (multicore_fifo_rvalid()){
      uint32_t msg = multicore_fifo_pop_blocking();   
      if(msg & SET_DUTY){
	timeout_us = timeouts_us[(msg & DUTY_MASK)];
      }
      else if(msg & IS_AC_ON){
	bool is_ac_on = (time_us_64()-prev_zerocross_time < PERIOD_1_25_US);
	multicore_fifo_push_blocking(is_ac_on);
      }
    }
    
    // If we crossed 0, schedule the next two switches
    if(zerocross_time != 0){
      uint64_t shifted_zerocross_time = zerocross_time - zerocross_delay;
      zerocross_time = 0;
      
      if(shifted_zerocross_time - prev_zerocross_time > PERIOD_0_50_US){
	// Schedule stop time
	uint64_t stop_time = shifted_zerocross_time + PERIOD_1_25_US;
	add_alarm_at(stop_time, &stop, &out_pin, false);
	
	// Schedule start time 
	if (timeout_us != 0){
	  uint64_t start_time = shifted_zerocross_time + PERIOD_1_00_US - timeout_us;
	  add_alarm_at(start_time, &start, &out_pin, false);
	}
      }
      prev_zerocross_time = shifted_zerocross_time;
    }
  }
}

/**
 * Called from core 0. Launches core 1 and passes it the required data.
 */
void phasecontrol_setup(PHASECONTROL_CONFIG * config) {
  multicore_launch_core1(phasecontrol_loop);
  sleep_ms(10); 
  // Pack configuration data
  uint32_t config_data = config->trigger;
  config_data = (config_data<<8) | config->zero_cross_pin;
  config_data = (config_data<<8) | config->out_pin;

  // Push data to core1
  multicore_fifo_push_blocking(config_data);
  multicore_fifo_push_blocking(config->zero_cross_delay);

  // Wait for everything to get setup
  sleep_ms(10); 
  return;
}

/**
 * Write the target duty cycle to core1
 */
void phasecontrol_set_duty_cycle(uint8_t duty_cycle){
  multicore_fifo_push_blocking(SET_DUTY | duty_cycle);
}

/**
 * Sends a request to core1 for ac status and then waits for and 
 *returns the response
 */
bool phasecontrol_is_ac_hot(){
  multicore_fifo_push_blocking(IS_AC_ON);
  while(!multicore_fifo_rvalid()){
    tight_loop_contents();
  }
  return multicore_fifo_pop_blocking();
}
