#pragma once
#include <chrono>
#include <cmath>

using time_us_t = std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>;

struct time_split_t
{
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
	int millisecond;
};

time_split_t time_split(time_us_t time_us);