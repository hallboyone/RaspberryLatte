#include "hardware/clocks.h"

#include "hx711.h"
#include "hx711.pio.h"

#include "uart_bridge.h"
#include "maintainer.h"
#include "status_ids.h"

static PIO _pio;
static uint _sm;
static uint32_t _latest_weight;

static inline void hx711_program_init(uint8_t dat_pin, uint8_t clk_pin, uint offset) {
    pio_gpio_init(_pio, dat_pin);
    pio_gpio_init(_pio, clk_pin);
    pio_sm_set_consecutive_pindirs(_pio, _sm, clk_pin, 1, true);
    pio_sm_set_consecutive_pindirs(_pio, _sm, dat_pin, 1, false);

    pio_sm_config c = hx711_program_get_default_config(offset);
    
    sm_config_set_sideset_pins(&c, clk_pin);
    sm_config_set_jmp_pin(&c, dat_pin);
    sm_config_set_in_pins(&c, dat_pin);
    
    sm_config_set_in_shift(&c, false, false, 0);
    sm_config_set_out_shift(&c, false, true, 32);

    // Each bit should take 2us over 12 cycles = 6cycles per us = 6e6Hz
    float div = (float)clock_get_hz(clk_sys)/6000000.;
    sm_config_set_clkdiv(&c, div);
    
    pio_sm_init(_pio, _sm, offset, &c);
    pio_sm_set_enabled(_pio, _sm, true);
}

static void hx711_maintainer(){
    while(!pio_sm_is_rx_fifo_empty(_pio, _sm)){
        _latest_weight = pio_sm_get_blocking(_pio, _sm);
    }
}

static void hx711_read_handler(int * msg, int len){
    int response [3];
    response[0] = (_latest_weight >> 16) & 0xFF;
    response[1] = (_latest_weight >>  8) & 0xFF;
    response[2] = (_latest_weight >>  0) & 0xFF;
    sendMessageWithStatus(MSG_ID_GET_PRESSURE, SUCCESS, response, 3);
    return;
}

void hx711_setup(uint8_t pio_num, uint8_t dat_pin, uint8_t clk_pin){
  _pio =  (pio_num==0) ? pio0 : pio1;
  uint offset = pio_add_program(_pio, &hx711_program);
  _sm = pio_claim_unused_sm(_pio, true);
  hx711_program_init(dat_pin, clk_pin, offset);

  registerHandler(MSG_ID_GET_WEIGHT, &hx711_read_handler);
  registerMaintainer(&hx711_maintainer);
}

