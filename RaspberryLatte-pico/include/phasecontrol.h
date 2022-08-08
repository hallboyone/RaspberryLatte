#include "pico/stdlib.h"

#include "uart_bridge.h"

#define ZEROCROSS_EVENT_RISING   0x08
#define ZEROCROSS_EVENT_FALLING  0x04

/**
 * Structure holding the configuration values of a phase controller for and
 * inductive load.
 */
typedef struct {
  uint8_t event;             // RISING or FALLING
  uint8_t zerocross_pin;     // GPIO that senses event at every zerocrossing
  int64_t zerocross_shift;   // Time between zerocross trigger and actual zero cross
  uint8_t out_pin;           // Load output pin

  uint64_t _zerocross_time;
  uint8_t _timeout_idx;
} phasecontrol;

/**
 * \brief Setup for phasecontrol. Pins are configured and a callback is attached to the zerocross pin.
 * 
 * \param p A pointer to a phasecontrol struct representing the object.
 * \param zerocross_pin Pin that senses zero crossing
 * \param out_pin Pin that switches the load
 * \param zerocross_shift Time in us that the zerocross is from the sensing time.
 * \param event Event to trigger zerocross on. Either ZEROCROSS_EVENT_RISING or ZEROCROSS_EVENT_FALLING. 
 */
void phasecontrol_setup(phasecontrol * p, uint8_t zerocross_pin, uint8_t out_pin, int32_t zerocross_shift, uint8_t event);

/**
 * \brief Update the duty cycle. If value is out of range (0<=val<=127), it is clipped.
 * 
 * \param duty_cycle New duty cycle value between 0 and 127 inclusive.
 */
int phasecontrol_set_duty_cycle(phasecontrol * p, uint8_t duty_cycle);

/**
 * \brief Check if zerocross pin has triggered in the last 16766us (period of 60Hz signal plus 100us), 
 * indicating active AC.
 * 
 * \return true if zerocross pin triggered in the last 16,766us. False otherwise.
 */
bool phasecontrol_is_ac_hot(phasecontrol * p);

/**
 * \brief Callback that reads if the ac is on for the phasecontrol struct pointed to by local_data.
 * 
 * \param id The ID of the callback. Each registered callback must have a unique callback ID.
 * \param local_data Void pointer which MUST point at an phasecontrol object.
 * \param uart_data Pointer to data sent over UART. Since this is a read callback, no data is needed.
 * \param uart_data_len Number of bytes in uart_data. Since this is a read callback, this should be 0.
 */
void phasecontrol_is_ac_hot_uart_callback(message_id id, void * local_data, int * uart_data, int uart_data_len);

/**
 * \brief Callback that sets the duty cycle for the phasecontrol struct pointed to by local_data. The
 * duty cycle after clipping is returned over the UART bridge.
 * 
 * \param id The ID of the callback. Each registered callback must have a unique callback ID.
 * \param local_data Void pointer which MUST point at an phasecontrol object.
 * \param uart_data Pointer to data sent over UART. Since this is a read callback, no data is needed.
 * \param uart_data_len Number of bytes in uart_data. Since this is a read callback, this should be 0.
 */
void phasecontrol_set_duty_uart_callback(message_id id, void * local_data, int * uart_data, int uart_data_len);