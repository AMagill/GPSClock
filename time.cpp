#include "time.hpp"

Time_Parts time_split(Time_us time_us)
{
	using namespace std::chrono;
	
	Time_Parts time;
	
	auto date_days = floor<days>(time_us);
	year_month_day ymd{date_days};
	hh_mm_ss time_hms{floor<milliseconds>(time_us-date_days)};
	time.year        = static_cast<int>(ymd.year());
	time.month       = static_cast<unsigned int>(ymd.month());
	time.day         = static_cast<unsigned int>(ymd.day());
	time.hour        = time_hms.hours()      / 1h;
	time.minute      = time_hms.minutes()    / 1min;
	time.second      = time_hms.seconds()    / 1s;
	time.millisecond = time_hms.subseconds() / 1ms;
	
	return time;
}