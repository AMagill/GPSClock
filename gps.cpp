#include "gps.h"
#include <charconv>
#include <chrono>

Gps::time_us Gps::time()
{
	using namespace std::chrono;

	uint64_t hw_time = to_us_since_boot(get_absolute_time());
	time_us time = time_us(microseconds(m_clock_offset_us)) + microseconds(hw_time);

  return time;
}

void Gps::on_rx(char ch)
{
	if (ch == '$')  // Start delimiter
		m_buf_pos = 0;
	else if (m_buf_pos == m_buf.size()) // Overrun
		return;
	
	if (m_buf_pos > 0 && m_buf[m_buf_pos-1] == '\r' && ch == '\n') // End delimiter
		handle_sentence(std::string_view(m_buf.data(), m_buf_pos-1));

	m_buf[m_buf_pos] = ch;
	m_buf_pos++;
}

void Gps::on_pps()
{
	uint64_t hw_time_us = to_us_since_boot(get_absolute_time());
	uint old_offset = m_phase_offset_us;
	m_phase_offset_us = hw_time_us % 1000000;
}

template <typename T>
static bool parse(std::string_view str, T& value, int base)
{
	auto result = std::from_chars(str.begin(), str.end(), value, base);
	return result.ec == std::errc() && result.ptr == str.end();
}

void Gps::handle_sentence(std::string_view sentence)
{
	// First validate the checksum
	if (sentence.size() < 9)  // Shortest possible valid sentence "$GPXXX*00"
		return;
	if (sentence[sentence.size() - 3] != '*')  // Expect a checksum delimiter here
		return;
	
	// Parse the checksum out of the last two bytes
	char checksum;
	if (!parse(sentence.substr(sentence.size() - 2), checksum, 16))
		return;

	// Checksum the sentence
	sentence = sentence.substr(1, sentence.size() - 4);  // Trim off start and checksum
	for (char ch : sentence)
		checksum ^= ch;

	// Should come out to zero when valid
	if (checksum != 0)
		return;

	// Now handle the payload.  We're only really interested in a couple fields from GPRMC.
	// $GPRMC,041826.000,A,3959.0864,N,10514.5749,W,0.11,95.04,271124,,,A*4C	
	// 0     1^^^time^^^2 3         4 5          6 7    8     9^date^ABC
	// We need the time in field 1, and date in field 9.
	uint8_t th, tm, ts;
	for (int field = 0; !sentence.empty(); field++)
	{
		using namespace std::chrono;

		// Trim a field off of the remaining sentence
		std::string_view value;
		if (size_t end = sentence.find(','); end != sentence.npos)
		{
			value = sentence.substr(0, end);
			sentence = sentence.substr(end + 1);
		}
		else
		{
			value = sentence;
			sentence = {};
		}

		switch (field)
		{
		case 0:
			if (value != "GPRMC")
				return;
			break;
		case 1:
			// Expect time, like: "041826.000"
			if (value.size() < 6)  // Ignoring fraction, using PPS for that
				return;
			if (!parse(value.substr(0, 2), th, 10) ||
			    !parse(value.substr(2, 2), tm, 10) ||
			    !parse(value.substr(4, 2), ts, 10))
				return;
			break;
		case 9:
		{
			if (value.size() != 6)
				return;
			int dy;
			uint dm, dd;
			if (!parse(value.substr(0, 2), dd, 10) ||
					!parse(value.substr(2, 2), dm, 10) ||
					!parse(value.substr(4, 2), dy, 10))
				return;

			// Calculate the offset between GPS time and system time
			time_us gps_time    = sys_days{year{2000+dy} / month{dm} / day{dd}} + hours{th} + minutes{tm} + seconds{ts};
			uint64_t hw_time_us = to_us_since_boot(get_absolute_time());
			m_clock_offset_us   = gps_time.time_since_epoch().count() - hw_time_us - m_phase_offset_us;
			break;
		}
		default:
			// Ignore
			break;
		}
	}
}
