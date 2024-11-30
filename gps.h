#include "pico/stdlib.h"
#include "hardware/uart.h"
#include <array>
#include <string_view>
#include <chrono>

class Gps
{
public:
	using time_us = std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>;

	time_us time();
	void on_rx(char ch);
	void on_pps();

private:
	void handle_sentence(std::string_view sentence);

	std::array<char, 82> m_buf;  // Maximum length allowed by standard
	uint m_buf_pos = -1;         // Start in overrun state
	uint64_t m_clock_offset_us = 0;
	uint m_phase_offset_us = 0;
};