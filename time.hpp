#pragma once
#include <chrono>
#include <cmath>

using Time_us = std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>;

struct Time_Parts
{
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
	int millisecond;
};

enum class Time_Quality
{
	INVALID,  // No idea of date or time
	LOW,      // Time was known once, but not recently
	MEDIUM,   // Time is fresh, but not PPS synced
	HIGH,     // Time is fresh, and PPS synced
};

Time_Parts time_split(Time_us time_us);