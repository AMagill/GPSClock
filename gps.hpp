#pragma once
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "time.hpp"
#include <array>
#include <string_view>

void gps_init(uart_inst_t* uart, uint baud, uint rx_pin, uint tx_pin);
void gps_on_pps();
Time_us gps_get_time();
Time_Quality gps_get_time_quality();
