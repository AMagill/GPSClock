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
	m_last_pps_time_us = to_us_since_boot(get_absolute_time());
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

	// Split the sentence into fields
	std::array<std::string_view, 12> fields;  // Could be more, but I don't care.  Increase if needed.
	for (size_t i = 0; i < fields.size(); i++)
	{
		if (size_t end = sentence.find(','); end != sentence.npos)
		{
			fields[i] = sentence.substr(0, end);
			sentence = sentence.substr(end + 1);
		}
		else
		{
			fields[i] = sentence;
			break;
		}
	}

	volatile int foo = 42;

	// Now handle the payload.  We're only really interested in a couple fields from GPRMC.
	// $GPRMC,041826.000,A,3959.0864,N,10514.5749,W,0.11,95.04,271124,,,A*4C	
	// 0     1^^^time^^^2 3         4 5          6 7    8     9^date^ABC
	// We need the time in field 1, and date in field 9.
	if (fields[0] == "GNRMC")
	{
		// Expect time, like: "041826.000"
		const std::string_view& time = fields[1];
		uint8_t th, tm, ts;
		if (time.size() < 6)  // Ignoring fraction, using PPS for that
			return;
		if (!parse(time.substr(0, 2), th, 10) ||
				!parse(time.substr(2, 2), tm, 10) ||
				!parse(time.substr(4, 2), ts, 10))
			return;

		// Expect date, like: "271124"
		const std::string_view& date = fields[9];
		if (date.size() != 6)
			return;
		int dy;
		uint dm, dd;
		if (!parse(date.substr(0, 2), dd, 10) ||
				!parse(date.substr(2, 2), dm, 10) ||
				!parse(date.substr(4, 2), dy, 10))
			return;

		// Make sure we've seen a PPS pulse recently
		uint64_t hw_time_us = to_us_since_boot(get_absolute_time());
		if (hw_time_us - m_last_pps_time_us > 1'000'000)  // More than a second since last PPS
			return;

		// Update the offset between GPS time and system time
		using namespace std::chrono;
		time_us gps_time  = sys_days{year{2000+dy} / month{dm} / day{dd}} + hours{th} + minutes{tm} + seconds{ts};
		uint64_t old_offset = m_clock_offset_us;
		m_clock_offset_us = gps_time.time_since_epoch().count() - m_last_pps_time_us;
		m_last_correction_us = m_clock_offset_us - old_offset;
	}
}
