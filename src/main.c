#include "pico/stdlib.h"
#include "pico/time.h"

#include "phasecontrol.h"
#include "physical_inputs.h"
#include "pressure_sensor.h"
#include "uart_bridge.h"
#include "hx711.pio.h"
#include "lmt01.pio.h"

void blink_io(){
  int timeout = 1000;
  int msg[2] = {0,0};
  while(true){
    msg[0] = getchar_timeout_us(0);
    if(msg[0] != PICO_ERROR_TIMEOUT){
      msg[1] = getchar_timeout_us(0);
      if(msg[1] != PICO_ERROR_TIMEOUT){
        timeout = msg[1]<<8 | msg[0];
      }
    }
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    sleep_ms(timeout);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    sleep_ms(timeout);
  }
}

int main(){
  stdio_init_all();

  // Read the settings over UART
  MachineSettings settings;
  while(!getSettings(&settings)) tight_loop_contents();

  // ============ Set up LED =============
  const uint LED_PIN = PICO_DEFAULT_LED_PIN;
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  // ========= Set up the scale ==========
  HX711 scale = {.pio_num = 0,
                 .dat_pin = 16,
                 .clk_pin = 17};
  //hx711_setup(&scale);

  // ======== Set up pressure sensor ========
  PressureSensor pressure_sensor = {.a_pin = 28};
  //pressure_sensor_setup(&pressure_sensor);

  // ======== Set up physical inputs ========
  PhysicalInputs switches = {.gpio_pump = 16,
			                       .gpio_dial = {17, 18, 19, 20}};
  physical_inputs_setup(&switches);

  // ======== Set up digital thermo ========
  LMT01 thermo = {.pio_num = 0,
                  .sig_pin = 15};
  //lmt01_setup(&thermo);

  // ======= Set up phase control =======
  PhasecontrolConfig pump_config = {.event           = FALLING,
				                            .zerocross_pin   = 15,
				                            .out_pin         = 14,
				                            .zerocross_shift = 300};
  //phasecontrol_setup(&pump_config);
  
  blink_io();

/*
  uint64_t payload = 0;
  bool led_state = false;
  while(1){
    packData(0, 1, 2, 3, &payload);
    gpio_put(LED_PIN, phasecontrol_is_ac_hot());
  }
  */
}
