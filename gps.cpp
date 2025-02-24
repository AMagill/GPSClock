#include "gps.hpp"
#include "hardware/uart.h"
#include <charconv>
#include <chrono>
#include <cstring>

static uart_inst_t* uart;
static std::array<uint8_t, 32> rx_buf;  // Needs to be at least as big as the largest message we expect
static uint         rx_buf_pos         = rx_buf.size();  // Start in overrun state
static uint64_t     clock_offset_us    = 0;
static uint64_t     last_pps_time_us   = 0;
static uint64_t     last_msg_time_us   = 0;
static int          last_correction_us = 0;
static uint32_t     time_accuracy         = 0xFFFFFFFF;

static std::pair<uint8_t, uint8_t> ubx_checksum(std::span<uint8_t> data)
{
	uint8_t ck_a = 0, ck_b = 0;
	for (uint8_t b : data)
	{
		ck_a += b;
		ck_b += ck_a;
	}
	return {ck_a, ck_b};
}

template <typename T>
T read_bytes(std::span<uint8_t>& data)
{
	T value;
	std::memcpy(&value, data.data(), sizeof(T));
	data = data.subspan(sizeof(T));
	return value;
}

template <typename T>
void skip_bytes(std::span<uint8_t>& data)
{
	data = data.subspan(sizeof(T));
}


static void handle_ubx(std::span<uint8_t> msg)
{
	if (msg.size() < 6)  // Shortest possible message with zero payload
		return;
	
	// First validate the checksum
	auto [ck_a, ck_b] = ubx_checksum(msg.subspan(0, msg.size()-2));
	if (ck_a != msg[msg.size()-2] || ck_b != msg[msg.size()-1])
		return;
	
	// We'll consume the message as we go
	msg = msg.subspan(0, msg.size()-2);

	uint64_t hw_time_us = to_us_since_boot(get_absolute_time());

	uint8_t cls = read_bytes<uint8_t>(msg);
	uint8_t id  = read_bytes<uint8_t>(msg);
	skip_bytes<uint16_t>(msg);  // Length

	// Now handle messages we're interested in.
	if (cls == 0x01 && id == 0x21 && msg.size() == 20)  // UBX-NAV-TIMEUTC
	{
		skip_bytes<uint32_t>(msg);  // iTOW
		uint32_t tAcc  = read_bytes<uint32_t>(msg);
		int32_t  nano  = read_bytes<int32_t>(msg);
		uint16_t dy    = read_bytes<uint16_t>(msg);
		uint8_t  dm    = read_bytes<uint8_t>(msg);
		uint8_t  dd    = read_bytes<uint8_t>(msg);
		uint8_t  th    = read_bytes<uint8_t>(msg);
		uint8_t  tm    = read_bytes<uint8_t>(msg);
		uint8_t  ts    = read_bytes<uint8_t>(msg);
		uint8_t  valid = read_bytes<uint8_t>(msg);

		// Assemble the time
		using namespace std::chrono;
		Time_us utc_time  = sys_days{year{dy} / month{dm} / day{dd}} + hours{th} + minutes{tm} + seconds{ts};
		uint64_t old_offset = clock_offset_us;
		last_msg_time_us    = hw_time_us;

		// Check how long it's been since the last PPS pulse
		if (hw_time_us - last_pps_time_us < 1'000'000)
		{	// Less than a second since last PPS.  We're going to ignore the
			// milliseconds in the message, and align the second to the PPS.
			clock_offset_us = utc_time.time_since_epoch().count() - last_pps_time_us;
		}
		else
		{	// More than a second since last PPS.  We'll use the message time.
			utc_time += microseconds(nano / 1000);
			clock_offset_us = utc_time.time_since_epoch().count() - hw_time_us;
		}

		last_correction_us = clock_offset_us - old_offset;
		printf("%+d\n", last_correction_us);
	}
	else if (cls == 0x01 && id == 0x22 && msg.size() == 20)  // UBX-NAV-CLOCK
	{
		skip_bytes<uint32_t>(msg);               // iTOW ms
		skip_bytes<int32_t>(msg);                // clkB ns
		skip_bytes<int32_t>(msg);                // clkD ns/s
		time_accuracy = read_bytes<uint32_t>(msg);  // tAcc ns
		skip_bytes<uint32_t>(msg);               // fAcc ps/s
	}
}

static void uart_rx_isr()
{
	while (uart_is_readable(uart))
	{
		char ch = uart_getc(uart);
		
		// We only care about UBX messages. They start with 0xB5, 0x62.
		// Use rx_buf_pos as a sort of state machine, invalidating when the frame looks bad.
		if (rx_buf_pos == 0 && ch != 0xB5)
				continue;
		if (rx_buf_pos == 1 && ch != 0x62)
		{
			rx_buf_pos = 0;
			continue;
		}
		if (rx_buf_pos >= rx_buf.size())
		{	// We've overrun the buffer.  Start over.
			rx_buf_pos = 0;
			continue;
		}

		rx_buf[rx_buf_pos++] = ch;
		if (rx_buf_pos > 6)
		{	// We have enough to check the length
			uint16_t len = rx_buf[4] | (rx_buf[5] << 8);
			if (rx_buf_pos == len + 8)
			{	// We have a complete message
				handle_ubx(std::span(rx_buf.data()+2, len+6));
				rx_buf_pos = 0;
			}
		}
	}
}

void gps_send_ubx(uint8_t cls, uint8_t id, std::initializer_list<uint8_t> payload)
{
	std::vector<uint8_t> buf;
	buf.reserve(8 + payload.size());
	buf.push_back(0xB5);
	buf.push_back(0x62);
	buf.push_back(cls);
	buf.push_back(id);
	uint16_t len = payload.size();
	buf.push_back(len & 0xFF);
	buf.push_back(len >> 8);
	std::copy(payload.begin(), payload.end(), std::back_inserter(buf));

	auto [ck_a, ck_b] = ubx_checksum(std::span(buf.begin()+2, buf.end()));
	buf.push_back(ck_a);
	buf.push_back(ck_b);

	uart_write_blocking(uart, buf.data(), buf.size());
}

void gps_init_io(uart_inst_t* uart, uint baud, uint rx_pin, uint tx_pin)
{
	// Set up the GPS UART
	::uart = uart;
	uart_init(uart, baud);
	gpio_set_function(rx_pin, GPIO_FUNC_UART);
	gpio_set_function(tx_pin, GPIO_FUNC_UART);
	uart_set_hw_flow(uart, false, false);  // No CTS, no RTS
	uart_set_format(uart, 8, 1, UART_PARITY_NONE);
	
	uint gps_uart_irq = UART_IRQ_NUM(uart);
	irq_set_exclusive_handler(gps_uart_irq, uart_rx_isr);
	irq_set_enabled(gps_uart_irq, true);
	uart_set_irq_enables(uart, true, false);
}

void gps_init_comms()
{
	gps_send_ubx(0x06, 0x01, {0xF0, 0x00, 0x00});  // UBX-CFG-MSG: GGA off
	gps_send_ubx(0x06, 0x01, {0xF0, 0x01, 0x00});  // UBX-CFG-MSG: GLL off
	gps_send_ubx(0x06, 0x01, {0xF0, 0x02, 0x00});  // UBX-CFG-MSG: GSA off
	gps_send_ubx(0x06, 0x01, {0xF0, 0x03, 0x00});  // UBX-CFG-MSG: GSV off
	gps_send_ubx(0x06, 0x01, {0xF0, 0x04, 0x00});  // UBX-CFG-MSG: RMC off
	gps_send_ubx(0x06, 0x01, {0xF0, 0x05, 0x00});  // UBX-CFG-MSG: VTG off
	gps_send_ubx(0x06, 0x01, {0x01, 0x21, 0x01});  // UBX-CFG-MSG: UBX-NAV-TIMEUTC 1Hz
	gps_send_ubx(0x06, 0x01, {0x01, 0x22, 0x01});  // UBX-CFG-MSG: UBX-NAV-CLOCK   1Hz
	gps_send_ubx(0x06, 0x31, {                     // UBX-CFG-TP5:
			0x00,        // TIMEPULSE                 0
			0x01,        // Message version           1 
			0x00, 0x00,  // Reserved
			0x00, 0x00,  // Antenna cable delay       0 ns
			0x00, 0x00,  // RF group delay            0 ns
			 1, 0, 0, 0, // Frequency                 1 Hz
			 1, 0, 0, 0, // Locked frequency          1 Hz
			10, 0, 0, 0, // Pulse length             10 us
			10, 0, 0, 0, // Locked pulse length      10 us
			 0, 0, 0, 0, // User configurable delay   0 ns
			             // Flags:  (Bits 0-7)
			 1<<0 |      //   Active
			 1<<1 |      //   Locked to GPS time
			 1<<3 |      //   Frequency units Hz
			 1<<4 |      //   Time units us
			 1<<5 |      //   Align to top of second
			 1<<6,       //   Rising edge
			             // Flags:  (Bits 8-15)
			 1<<3,       //   Can lose sync
			 0, 0,       // Flags;  (Bits 16-31 unused)
	});
}

Time_us gps_get_time()
{
	using namespace std::chrono;

	uint64_t hw_time = to_us_since_boot(get_absolute_time());
	Time_us time = Time_us(microseconds(clock_offset_us)) + microseconds(hw_time);

	return time;
}

uint32_t gps_get_time_accuracy_ns()
{
	return time_accuracy;
}

void gps_on_pps()
{
	last_pps_time_us = to_us_since_boot(get_absolute_time());
}
