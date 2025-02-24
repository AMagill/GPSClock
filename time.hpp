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

Time_Parts time_split(Time_us time_us);