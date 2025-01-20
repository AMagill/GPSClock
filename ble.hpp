#pragma once
#include "time.hpp"
#include <cstdint>
#include <string>

void ble_init();
void ble_tick_time(const time_split_t& time);
uint8_t ble_get_brightness();
void    ble_set_brightness(uint8_t bright);