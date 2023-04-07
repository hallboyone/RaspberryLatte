/**
 * \file phasecontrol.c
 * \author Richard Hall (hallboyone@icloud.com)
 * \version 0.1
 * \date 2022-08-16
 * \brief Phase Control source
 * 
 * \ingroup phasecontrol
 */
#include "utils/phasecontrol.h"

#include "pico/time.h"
#include <stdlib.h>

#include "utils/gpio_multi_callback.h"
#include "utils/macros.h"

/**
 * Structure holding the configuration values of a phase controller for and
 * inductive load.
 */
typedef struct phasecontrol_s{
  uint8_t event;            /**< The event to trigger on. Should be either ZEROCROSS_EVENT_RISING or ZEROCROSS_EVENT_FALLING */
  uint8_t zerocross_pin;    /**< GPIO attached to zerocross circuit */
  int64_t zerocross_shift;  /**< Time between zerocross trigger and actual zerocross */
  uint8_t out_pin;          /**< Load output pin. Usually attached to an SSR or relay */
  uint64_t zerocross_time;  /**< Time of the last zero-crossing. Used to determine if the AC is on. */
  uint8_t timeout_idx;      /**< The timeout (i.e. duty cycle). The smaller the number, the longer before load is switched on. */
} phasecontrol_;

static const uint64_t PERIOD_1_00 = 16667;
static const uint64_t PERIOD_0_75 = 12500;

/**
 * \brief Array of pointers to each of the configured controllers indexed by their zerocross pin.
 * 
 * Used by the zero-cross callbacks to update the corresponding phase controller
 */
static phasecontrol _configured_phasecontrollers [32]; 

/**
 * \brief All possible timeouts in ms.
 * 
 * Spaced so that the area under a 60Hz sine curve is split into 127 equal boxes.
 */
static const uint16_t _timeouts_us[128] =
  {8333,7862,7666,7515,7387,7274,7171,7076,6987,6904,6824,6749,6676,6606,6538,6472,
   6408,6346,6286,6226,6168,6112,6056,6001,5947,5895,5842,5791,5740,5690,5641,5592,
   5544,5496,5448,5401,5355,5309,5263,5217,5172,5127,5083,5039,4995,4951,4907,4864,
   4821,4778,4735,4692,4650,4607,4565,4523,4481,4439,4397,4355,4313,4271,4229,4188,
   4146,4104,4062,4020,3979,3937,3895,3853,3811,3768,3726,3684,3641,3598,3556,3513,
   3469,3426,3382,3339,3295,3250,3206,3161,3116,3071,3025,2979,2932,2885,2838,2790,
   2741,2693,2643,2593,2542,2491,2439,2386,2332,2277,2222,2165,2107,2048,1987,1925,
   1861,1795,1728,1658,1585,1509,1430,1346,1257,1162,1060, 947, 819, 668, 471,   0};	

/**
 * \brief Alarm callback writing 0 to the output GPIO to disable SSR or other switch.
 */
static int64_t _phasecontrol_set_output_low(int32_t alarm_num, void * data){
  UNUSED_PARAMETER(alarm_num);
  gpio_put(((phasecontrol_*)data)->out_pin, 0);
  return 0;
}

/**
 * \brief Alarm callback writing 1 to the output GPIO to trigger SSR or other switch.
 */
static int64_t _phasecontrol_set_output_high(int32_t alarm_num, void * data){
  UNUSED_PARAMETER(alarm_num);
  gpio_put(((phasecontrol_*)data)->out_pin, 1);
  return 0;
}

/**
 * \brief ISR for zerocross pin. Schedules alarms to turn the output pin on (after some delay)
 * of off (after 0.75 a period).
 */
static void _phasecontrol_switch_scheduler(uint gpio, uint32_t events, void * data){
  UNUSED_PARAMETER(events);
  volatile phasecontrol_ * p = (phasecontrol_*)data;
  const uint64_t cur_time = time_us_64();
  if(p->zerocross_time + PERIOD_0_75 < cur_time){
    p->zerocross_time = cur_time;
    if (p->timeout_idx > 0){
      // Schedule stop time after 0.75 period
      add_alarm_in_us(p->zerocross_shift + PERIOD_0_75, &_phasecontrol_set_output_low, _configured_phasecontrollers[gpio], false);
      // Schedule start time after the given timeout
      add_alarm_in_us(p->zerocross_shift + _timeouts_us[p->timeout_idx], &_phasecontrol_set_output_high, _configured_phasecontrollers[gpio], true);
    }
  }
}

phasecontrol phasecontrol_setup(uint8_t zerocross_pin, uint8_t out_pin, int32_t zerocross_shift, uint8_t event){
  phasecontrol p = malloc(sizeof(phasecontrol_));

  _configured_phasecontrollers[zerocross_pin] = p;

  p->zerocross_pin = zerocross_pin;
  p->out_pin = out_pin;
  p->zerocross_shift = zerocross_shift;
  p->event = event;

  p->zerocross_time = 0;
  p->timeout_idx = 0;

  // Setup SSR output pin
  gpio_init(p->out_pin);
  gpio_set_dir(p->out_pin, GPIO_OUT);

  // Setup zero-cross input pin
  gpio_init(p->zerocross_pin);
  gpio_set_dir(p->zerocross_pin, GPIO_IN);
  gpio_set_pulls(p->zerocross_pin, false, true);

  gpio_multi_callback_attach(p->zerocross_pin, p->event, true, &_phasecontrol_switch_scheduler, p);
  return p;
}

int phasecontrol_set_duty_cycle(phasecontrol p, uint8_t duty_cycle){
  if(duty_cycle>127) duty_cycle = 127;
  p->timeout_idx = duty_cycle;
  return p->timeout_idx;
}

bool phasecontrol_is_ac_hot(phasecontrol p){
  return p->zerocross_time + PERIOD_1_00 + 100 > time_us_64();
}

void phasecontrol_deinit(phasecontrol p){
  free(p);
}