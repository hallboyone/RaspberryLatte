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
  {0   , 471 , 668 , 819 , 947 , 1060, 1162, 1257, 1346, 1430,
   1509, 1585, 1658, 1728, 1795, 1861, 1925, 1987, 2048, 2107,
   2165, 2222, 2277, 2332, 2386, 2439, 2491, 2542, 2593, 2643,
   2693, 2741, 2790, 2838, 2885, 2932, 2979, 3025, 3071, 3116,
   3161, 3206, 3250, 3295, 3339, 3382, 3426, 3469, 3513, 3556,
   3598, 3641, 3684, 3726, 3768, 3811, 3853, 3895, 3937, 3979,
   4020, 4062, 4104, 4146, 4188, 4229, 4271, 4313, 4355, 4397,
   4439, 4481, 4523, 4565, 4607, 4650, 4692, 4735, 4778, 4821,
   4864, 4907, 4951, 4995, 5039, 5083, 5127, 5172, 5217, 5263,
   5309, 5355, 5401, 5448, 5496, 5544, 5592, 5641, 5690, 5740,
   5791, 5842, 5895, 5947, 6001, 6056, 6112, 6168, 6226, 6286,
   6346, 6408, 6472, 6538, 6606, 6676, 6749, 6824, 6904, 6987,
   7076, 7171, 7274, 7387, 7515, 7666, 7862, 8333};

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
 * loop. This loop check for a new duty cycle and data requests from core0.
 */
static void phasecontrol_loop() {  
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
