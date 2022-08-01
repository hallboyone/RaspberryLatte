#include "pico/time.h"

#include "phasecontrol.h"
#include "uart_bridge.h"
#include "status_ids.h"

#define PERIOD_1_00        16667
#define PERIOD_0_75        12500
#define PERIOD_0_50         8333

// Pointer to a constant configuration object. 
const PhasecontrolConfig * config;

// Timestamp of last zerocross time
static volatile uint64_t   zerocross_time = 0;

// All possible timeouts. Spaced so that the area under the curve is split into 127 equal boxes.
const uint16_t timeouts_us[128] =
  {8333,7862,7666,7515,7387,7274,7171,7076,6987,6904,6824,6749,6676,6606,6538,6472,
   6408,6346,6286,6226,6168,6112,6056,6001,5947,5895,5842,5791,5740,5690,5641,5592,
   5544,5496,5448,5401,5355,5309,5263,5217,5172,5127,5083,5039,4995,4951,4907,4864,
   4821,4778,4735,4692,4650,4607,4565,4523,4481,4439,4397,4355,4313,4271,4229,4188,
   4146,4104,4062,4020,3979,3937,3895,3853,3811,3768,3726,3684,3641,3598,3556,3513,
   3469,3426,3382,3339,3295,3250,3206,3161,3116,3071,3025,2979,2932,2885,2838,2790,
   2741,2693,2643,2593,2542,2491,2439,2386,2332,2277,2222,2165,2107,2048,1987,1925,
   1861,1795,1728,1658,1585,1509,1430,1346,1257,1162,1060, 947, 819, 668, 471,   0};	

// Index of current timeout drawn from timeouts_us
static volatile uint8_t timeout_idx = 0;  

/**
 * Parses a single byte message sent over UART with id MSG_ID_SET_PUMP
 * 
 * \param value a pointer to a single integer containing the data from message
 * \param len length of data array. Must be 1. 
 */
static void phasecontrol_set_duty_cycle_handler(int* value, int len){
  if(len==1){
    phasecontrol_set_duty_cycle(*value);
    sendMessageWithStatus(MSG_ID_SET_PUMP, SUCCESS, NULL, 0);
  } else {
    sendMessageWithStatus(MSG_ID_SET_PUMP, MSG_FORMAT_ERROR, NULL, 0);
  }
}

/**
 * Returns 1 over UART if AC is hot. 0 else.
 * 
 * \param value Pointer to an unused integer array
 * \param len Length of unused integer array
 */
static void phasecontrol_is_ac_hot_handler(int* value, int len){
  int response = phasecontrol_is_ac_hot();
  sendMessageWithStatus(MSG_ID_GET_AC_ON, SUCCESS, &response, 1);
}

/**
 * \brief Alarm callback writing 0 to the output GPIO to disable SSR or other switch.
 */
int64_t phasecontrol_set_output_low(int32_t alarm_num, void * data){
  gpio_put(config->out_pin, 0);
  return 0;
}

/**
 * \brief Alarm callback writing 1 to the output GPIO to trigger SSR or other switch.
 */
int64_t phasecontrol_set_output_high(int32_t alarm_num, void * data){
  gpio_put(config->out_pin, 1);
  return 0;
}

/**
 * \brief ISR for zerocross pin. Schedules alarms to turn the output pin on (after some delay)
 * of off (after 0.75 a period).
 */
void phasecontrol_switch_scheduler(uint gpio, uint32_t events){
  // Make sure we aren't re-sensing the same zerocrossing
  if(zerocross_time + PERIOD_0_75 < time_us_64()){
    zerocross_time = time_us_64();
    if (timeout_idx > 0){
      // Schedule stop time after 0.75 period
      add_alarm_in_us(config->zerocross_shift + PERIOD_0_75, &phasecontrol_set_output_low, NULL, false);
      // Schedule start time after the given timeout
      add_alarm_in_us(config->zerocross_shift + timeouts_us[timeout_idx], &phasecontrol_set_output_high, NULL, true);
    }
  }
}

/**
 * \brief Setup for phasecontrol. Pins are configured and a callback is attached to the zerocross pin.
 * The phasecontroller is also registered with the UART bridge. 
 * 
 * \param user_config A constant configuration object containing the desired pin number, and other fields. 
 */
void phasecontrol_setup(const PhasecontrolConfig * user_config) {
  config = user_config;

  // Setup SSR output pin
  gpio_init(config->out_pin);
  gpio_set_dir(config->out_pin, GPIO_OUT);

  // Setup zero-cross input pin
  gpio_init(config->zerocross_pin);
  gpio_set_dir(config->zerocross_pin, GPIO_IN);
  gpio_set_pulls(config->zerocross_pin, false, true);
  gpio_set_irq_enabled_with_callback(config->zerocross_pin, config->event, true, &phasecontrol_switch_scheduler);

  // Setup UART handlers
  registerHandler(MSG_ID_SET_PUMP, &phasecontrol_set_duty_cycle_handler);
  registerHandler(MSG_ID_GET_AC_ON, &phasecontrol_is_ac_hot_handler);
  return;
}

/**
 * \brief Update the duty cycle. If value is out of range (0<=val<=127), it is clipped.
 * 
 * \param duty_cycle New duty cycle value between 0 and 127 inclusive.
 */
void phasecontrol_set_duty_cycle(uint8_t duty_cycle){
  if(duty_cycle>127) duty_cycle = 127;
  timeout_idx = duty_cycle;
}

/**
 * \brief Check if zerocross pin has triggered in the last 16766us (period of 60Hz signal plus 100us), 
 * indicating active AC.
 * 
 * \return true if zerocross pin triggered in the last 16,766us. False otherwise.
 */
bool phasecontrol_is_ac_hot(){
  return zerocross_time + PERIOD_1_00 + 100 < time_us_64();
}