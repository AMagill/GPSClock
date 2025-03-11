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
void  ble_tick_time(const Time_Parts& time, uint32_t time_acc);
void  ble_set_command_cb(std::function<void(BLECommand)> cb);
uint8_t ble_get_id();