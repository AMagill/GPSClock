#pragma once
#include "time.hpp"
#include <cstdint>
#include <array>
#include <string_view>
#include "hardware/pio.h"

void disp_init(PIO pio, uint tx_pin, uint clk_pin, uint latch_pin);
void disp_latch();
void disp_send_control();
void disp_send_leds();
void disp_set_brightness(uint8_t bright);
void disp_set_digit(uint digit, uint8_t value, bool dp);
void disp_set_colons(bool on);
void disp_set_time(const Time_Parts& time, Time_Quality quality);