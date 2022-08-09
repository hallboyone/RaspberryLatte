#include "pico/time.h"

#include "phasecontrol.h"
#include "status_ids.h"

#define PERIOD_1_00        16667
#define PERIOD_0_75        12500
#define PERIOD_0_50         8333

phasecontrol * _configured_phasecontrollers [32]; //indexed by their zerocross pin.

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

/**
 * \brief Alarm callback writing 0 to the output GPIO to disable SSR or other switch.
 */
int64_t phasecontrol_set_output_low(int32_t alarm_num, void * data){
  phasecontrol* p = (phasecontrol*)data;
  gpio_put(p->out_pin, 0);
  return 0;
}

/**
 * \brief Alarm callback writing 1 to the output GPIO to trigger SSR or other switch.
 */
int64_t phasecontrol_set_output_high(int32_t alarm_num, void * data){
  phasecontrol* p = (phasecontrol*)data;
  gpio_put(p->out_pin, 1);
  return 0;
}

/**
 * \brief ISR for zerocross pin. Schedules alarms to turn the output pin on (after some delay)
 * of off (after 0.75 a period).
 */
void phasecontrol_switch_scheduler(uint gpio, uint32_t events){
  phasecontrol * p = _configured_phasecontrollers[gpio];
  // Make sure we aren't re-sensing the same zero crossing
  if(p->_zerocross_time + PERIOD_0_75 < time_us_64()){
    p->_zerocross_time = time_us_64();
    if (p->_timeout_idx > 0){
      // Schedule stop time after 0.75 period
      add_alarm_in_us(p->_zerocross_time + PERIOD_0_75, &phasecontrol_set_output_low, p, false);
      // Schedule start time after the given timeout
      add_alarm_in_us(p->_zerocross_time + timeouts_us[p->_timeout_idx], &phasecontrol_set_output_high, p, true);
    }
  }
}

/**
 * \brief Setup for phasecontrol. Pins are configured and a callback is attached to the zerocross pin.
 * 
 * \param p A pointer to a phasecontrol struct representing the object.
 * \param zerocross_pin Pin that senses zero crossing
 * \param out_pin Pin that switches the load
 * \param zerocross_shift Time in us that the zerocross is from the sensing time.
 * \param event Event to trigger zerocross on. Either ZEROCROSS_EVENT_RISING or ZEROCROSS_EVENT_FALLING. 
 */
void phasecontrol_setup(phasecontrol * p, uint8_t zerocross_pin, uint8_t out_pin, int32_t zerocross_shift, uint8_t event){
  _configured_phasecontrollers[zerocross_pin] = p;

  p->zerocross_pin = zerocross_pin;
  p->out_pin = out_pin;
  p->zerocross_shift = zerocross_shift;
  p->event = event;

  p->_zerocross_time = 0;

  // Setup SSR output pin
  gpio_init(p->out_pin);
  gpio_set_dir(p->out_pin, GPIO_OUT);

  // Setup zero-cross input pin
  gpio_init(p->zerocross_pin);
  gpio_set_dir(p->zerocross_pin, GPIO_IN);
  gpio_set_pulls(p->zerocross_pin, false, true);
  gpio_set_irq_enabled_with_callback(p->zerocross_pin, p->event, true, &phasecontrol_switch_scheduler);
  return;
}

/**
 * \brief Update the duty cycle. If value is out of range (0<=val<=127), it is clipped.
 * 
 * \param p Pointer to phase control object that will be updated
 * \param duty_cycle New duty cycle value between 0 and 127 inclusive.
 * 
 * \returns The duty cycle after clipping.
 */
int phasecontrol_set_duty_cycle(phasecontrol * p, uint8_t duty_cycle){
  if(duty_cycle>127) duty_cycle = 127;
  p->_timeout_idx = duty_cycle;
  return p->_timeout_idx;
}

/**
 * \brief Check if zerocross pin has triggered in the last 16766us (period of 60Hz signal plus 100us), 
 * indicating active AC.
 * 
 * \param p Pointer to phase control object that will be read.
 * 
 * \return true if zerocross pin triggered in the last 16,766us. False otherwise.
 */
bool phasecontrol_is_ac_hot(phasecontrol * p){
  return p->_zerocross_time + PERIOD_1_00 + 100 > time_us_64();
}

/**
 * \brief Callback that reads if the ac is on for the phasecontrol struct pointed to by local_data.
 * 
 * \param id The ID of the callback. Each registered callback must have a unique callback ID.
 * \param local_data Void pointer which MUST point at an phasecontrol object.
 * \param uart_data Pointer to data sent over UART. Since this is a read callback, no data is needed.
 * \param uart_data_len Number of bytes in uart_data. Since this is a read callback, this should be 0.
 */
void phasecontrol_is_ac_hot_uart_callback(message_id id, void * local_data, int * uart_data, int uart_data_len){
  int response = phasecontrol_is_ac_hot((phasecontrol*)local_data);
  sendMessageWithStatus(id, SUCCESS, &response, 1);
}

/**
 * \brief Callback that sets the duty cycle for the phasecontrol struct pointed to by local_data. The
 * duty cycle after clipping is returned over the UART bridge.
 * 
 * \param id The ID of the callback. Each registered callback must have a unique callback ID.
 * \param local_data Void pointer which MUST point at an phasecontrol object.
 * \param uart_data Pointer to data sent over UART. Since this is a read callback, no data is needed.
 * \param uart_data_len Number of bytes in uart_data. Since this is a read callback, this should be 0.
 */
void phasecontrol_set_duty_uart_callback(message_id id, void * local_data, int * uart_data, int uart_data_len){
  if(uart_data_len==1){
    int response = phasecontrol_set_duty_cycle((phasecontrol*)local_data, *uart_data);
    sendMessageWithStatus(id, SUCCESS, &response, 1);
  } else {
    sendMessageWithStatus(id, MSG_FORMAT_ERROR, NULL, 0);
  }
}
