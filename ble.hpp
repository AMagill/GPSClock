#pragma once
#include "time.hpp"
#include <cstdint>
#include <string>
#include <functional>

enum class BLECommand
{
    SAVE_SETTINGS,
};

void  ble_init();
void  ble_tick_time(const time_split_t& time);
void  ble_set_command_cb(std::function<void(BLECommand)> cb);