#include "pico/stdlib.h"
#include "hardware/uart.h"
#include <array>
#include <string_view>
#include <chrono>

using time_us_t = std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>;

void gps_init(uart_inst_t* uart, uint baud, uint rx_pin);
void gps_on_pps();
time_us_t gps_get_time();
