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
#define PERIOD_1_25        20833
#define PERIOD_1_00        16667
#define PERIOD_0_50        8333

// Message instructions. Or'd over value if applicable
#define SET_DUTY           0x10000000
#define DUTY_MASK          0x0000007F
#define IS_AC_ON           0x20000000

typedef struct {
  uint8_t trigger;
  uint8_t zerocross_pin;
  uint8_t out_pin;
  uint    zerocross_delay;
} PHASECONTROL_CONFIG;


static uint8_t out_pin;
static uint8_t zerocross_pin;
static uint8_t trigger;
static uint32_t zerocross_delay;

static uint64_t zerocross_time = 0;
static uint64_t prev_zerocross_time = 0;

static uint16_t timeout_us = 0;
const uint16_t timeouts_us[128] =
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
   8333, 8333, 8333, 8333, 8333, 8333, 8333, 8333, 8333,
   8333, 8333, 8333, 8333, 8333, 8333, 8333, 8333, 8333,
   8333, 8333, 8333, 8333, 8333, 8333, 8333, 8333};

static int64_t stop(int32_t alarm_num, void * data){
  gpio_put(out_pin, 0);
  return 0;
}

static int64_t start(int32_t alarm_num, void * data){
  gpio_put(out_pin, 1);
  return 0;
}

static void switch_scheduler(uint gpio, uint32_t events){
  prev_zerocross_time = zerocross_time;
  zerocross_time = time_us_64() - zerocross_delay;
  if(zerocross_time - prev_zerocross_time > PERIOD_1_00 - 100) {
    // Schedule stop time
    add_alarm_at(zerocross_time+PERIOD_1_25, &stop, NULL, false);
    
    // Schedule start time 
    if (timeout_us != 0) {
      add_alarm_at(zerocross_time+PERIOD_1_00-timeout_us, &start, NULL, false);
    }
  }
}

/**
 * The main loop running on core1. After getting the configuration values from
 * core0, it sets up the pins and the zerocross interrupt and enters an infinite
 * loop. This loop check for a new duty cycle and data requests from core0 and 
 * schedules switching times if a zerocross was detected.
 */
static void phasecontrol_loop() {
  // Get packed config values from core 0
  //uint32_t config_vals = multicore_fifo_pop_blocking();
  //out_pin = config_vals & 255;
  //zerocross_pin = (config_vals>>8) & 255;
  //trigger = (config_vals>>16) & 255;

  // Get zerocross time from core 0
  //zerocross_delay = multicore_fifo_pop_blocking();
  
  // Setup SSR output pin
  gpio_init(out_pin);
  gpio_set_dir(out_pin, GPIO_OUT);

  if(zerocross_delay == 100){
  // Setup zero-cross input pin
  gpio_init(zerocross_pin);
  gpio_set_dir(zerocross_pin, GPIO_IN);
  gpio_set_pulls(zerocross_pin, false, true);
  gpio_set_irq_enabled_with_callback(zerocross_pin,
				     trigger, true,
				     &switch_scheduler);
  }
  while (true) {
    // Handle data from core0
    if (multicore_fifo_rvalid()){
      uint32_t msg = multicore_fifo_pop_blocking();   
      if(msg & SET_DUTY){
	uint8_t duty_idx = (msg & DUTY_MASK);
	timeout_us = timeouts_us[duty_idx];
      }
      else if(msg & IS_AC_ON){
	bool is_ac_on = (time_us_64()-zerocross_time < PERIOD_1_25);
	multicore_fifo_push_blocking(is_ac_on);
      }
    }
  }
}




/* ===================================================================
 * ==================== FUNCTIONS FOR CORE 0 =========================
 * ===================================================================*/

/**
 * Called from core 0. Launches core 1 and passes it the required data.
 */
void phasecontrol_setup(PHASECONTROL_CONFIG * config) {
  out_pin = config->out_pin;
  zerocross_pin = config->zerocross_pin;
  zerocross_delay = config->zerocross_delay;
  trigger = config->trigger;
  
  multicore_launch_core1(phasecontrol_loop);
  /*
  sleep_ms(10); 
  // Pack configuration data
  uint32_t config_data = config->trigger;
  config_data = (config_data<<8) | config->zero_cross_pin;
  config_data = (config_data<<8) | config->out_pin;

  // Push data to core1
  multicore_fifo_push_blocking(config_data);
  //multicore_fifo_push_blocking(config->zero_cross_delay);

  // Wait for everything to get setup
  sleep_ms(10); */
  return;
}

/**
 * Write the target duty cycle to core1
 */
void phasecontrol_set_duty_cycle(uint8_t duty_cycle){
  uint32_t masked_msg = (uint32_t)duty_cycle | SET_DUTY;
  multicore_fifo_push_blocking(masked_msg);
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
