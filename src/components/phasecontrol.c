/**
 * Reads a duty cycle values from 0 to 100 over i2c and switches an ssr 
 * accordingly relative to zerocross times. Designed to provide adjustment
 * to ac-driven inductive loads where simple pwm with and ssr would result
 * in inductive voltage spikes
 */

#include "../include/phasecontrol.h"
#include "pico/time.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"

#include "uart_bridge.h"

#define PERIOD_1_25        20833
#define PERIOD_1_00        16667
#define PERIOD_0_75        12500
#define PERIOD_0_50        8333

static PhasecontrolConfig config;

static queue_t power_queue;
static queue_t ac_on_queue;

static uint64_t   zerocross_time = 0;
static bool       ac_on          = false;
static alarm_id_t off_alarm      = 0;

const uint16_t timeouts_us[128] =
  {8333,7862,7666,7515,7387,7274,7171,7076,6987,6904,
   6824,6749,6676,6606,6538,6472,6408,6346,6286,6226,
   6168,6112,6056,6001,5947,5895,5842,5791,5740,5690,
   5641,5592,5544,5496,5448,5401,5355,5309,5263,5217,
   5172,5127,5083,5039,4995,4951,4907,4864,4821,4778,
   4735,4692,4650,4607,4565,4523,4481,4439,4397,4355,
   4313,4271,4229,4188,4146,4104,4062,4020,3979,3937,
   3895,3853,3811,3768,3726,3684,3641,3598,3556,3513,
   3469,3426,3382,3339,3295,3250,3206,3161,3116,3071,
   3025,2979,2932,2885,2838,2790,2741,2693,2643,2593,
   2542,2491,2439,2386,2332,2277,2222,2165,2107,2048,
   1987,1925,1861,1795,1728,1658,1585,1509,1430,1346,
   1257,1162,1060,947, 819, 668, 471, 0};	
static int8_t timeout_idx = -1;

// Clears the queue and adds the new value.
void update_queue_s8(queue_t * q, int8_t new_val){
  while (!queue_is_empty(q)){
    int8_t dummy_val;
    queue_remove_blocking(q, &dummy_val);
  }
  queue_add_blocking(q, &new_val);
}

static int64_t stop(int32_t alarm_num, void * data){
  gpio_put(config.out_pin, 0);
  return 0;
}

static int64_t start(int32_t alarm_num, void * data){
  gpio_put(config.out_pin, 1);
  return 0;
}

static int64_t signal_off(int32_t alarm_num, void * data){
  // If this is the most recent off_alarm set, turn off. 
  if(alarm_num == off_alarm) update_queue_s8(&ac_on_queue, 0);
  return 0;
}

static void switch_scheduler(uint gpio, uint32_t events){
  // If it has been about 1 period since the last signal
  if(zerocross_time + PERIOD_1_00 - 100 < time_us_64()){
    // Zerocrossing indicates AC is on for at least one period
    update_queue_s8(&ac_on_queue, 1);
    off_alarm = add_alarm_in_us(config.zerocross_shift + PERIOD_1_00 + 100, &signal_off, NULL, true);
    
    if (timeout_idx > 0){
      // Schedule stop time after 0.75 period
      add_alarm_in_us(config.zerocross_shift+PERIOD_0_75, &stop, NULL, false);
    
      // Schedule start time after the given timeout
      add_alarm_in_us(config.zerocross_shift + timeouts_us[timeout_idx], &start, NULL, true);
    }
    zerocross_time = time_us_64();
  }
}

/**
 * The main loop running on core1. After getting the configuration values from
 * core0, it sets up the pins and the zerocross interrupt and enters an infinite
 * loop. This loop check for a new duty cycle and data requests from core0.
 */
static void phasecontrol_loop_core1() {  
  // Setup SSR output pin
  gpio_init(config.out_pin);
  gpio_set_dir(config.out_pin, GPIO_OUT);

  // Setup zero-cross input pin
  gpio_init(config.zerocross_pin);
  gpio_set_dir(config.zerocross_pin, GPIO_IN);
  gpio_set_pulls(config.zerocross_pin, false, true);
  gpio_set_irq_enabled_with_callback(config.zerocross_pin, config.event, true, &switch_scheduler);

  update_queue_s8(&ac_on_queue, 0);
  while (true) {
    if (!queue_is_empty(&power_queue)){
      // New power setting sent to core1
      queue_remove_blocking(&power_queue, &timeout_idx);
    }
  }
}

/* ===================================================================
 * ==================== FUNCTIONS FOR CORE 0 =========================
 * ===================================================================*/

/**
 * Parses a single byte message sent over UART with id MSG_ID_SET_PUMP
 * 
 * @param value a pointer to a single integer containing the data from message
 * @param len length of data array. Must be 1. 
 */
static void phasecontrol_set_duty_cycle_handler(int* value, int len){
  assert(len==1);
  phasecontrol_set_duty_cycle((*value<=127) ? *value : 127);
}

/**
 * Called from core 0. Launches core 1 and passes it the required data.
 */
void phasecontrol_setup(PhasecontrolConfig * user_config) {
  config = *user_config;
  
  // Set up queues and initialize the power queue with -1
  queue_init(&power_queue, sizeof(int8_t), 1);
  queue_init(&ac_on_queue, sizeof(int8_t), 1);
  update_queue_s8(&power_queue, -1);

  // Launch core1 and wait for it to get set up. 
  multicore_launch_core1(phasecontrol_loop_core1);
  while(queue_is_empty(&ac_on_queue)){
    tight_loop_contents();
  }

  // Setup UART handler
  assignHandler(MSG_ID_SET_PUMP, &phasecontrol_set_duty_cycle_handler);
  return;
}

/**
 * Write the target duty cycle from 0 to 127 to core1 with 0 being off. 
 */
void phasecontrol_set_duty_cycle(int8_t duty_cycle){
  update_queue_s8(&power_queue, duty_cycle);
}

/**
 * Returns true if a zerocrossing has been sensed in the last 16,666 us
 */
bool phasecontrol_is_ac_hot(){
  int8_t tf = 1;
  queue_peek_blocking(&ac_on_queue, &tf);
  return tf;
}
