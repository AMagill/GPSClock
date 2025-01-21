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
void  ble_tick_time(const Time_Parts& time);
void  ble_set_command_cb(std::function<void(BLECommand)> cb);