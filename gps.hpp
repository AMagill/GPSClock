#pragma once
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "time.hpp"
#include <array>
#include <string_view>

void gps_init_io(uart_inst_t* uart, uint baud, uint rx_pin, uint tx_pin);
void gps_init_comms();
void gps_on_pps();
Time_us gps_get_time();
uint32_t gps_get_time_accuracy_ns();
