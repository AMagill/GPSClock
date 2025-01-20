#include "display.hpp"
#include "pico/stdlib.h"
#include "tlc5952.pio.h"

static constexpr uint sm = 0;
static constexpr uint num_chips = 6;

static PIO  pio;
static uint latch_pin;
static uint offset;
static uint32_t control_buffer;
static std::array<uint32_t, num_chips> onoff_buffer;


void disp_init(PIO pio, uint tx_pin, uint clk_pin, uint latch_pin)
{
	::pio = pio;
	::latch_pin = latch_pin;

	gpio_init(latch_pin);
	gpio_set_dir(latch_pin, true);
	gpio_put(latch_pin, false);

	offset = pio_add_program(pio, &tlc5952_write_program);
	tlc5952_write_program_init(pio, sm, offset, tx_pin, clk_pin, latch_pin);
}

void disp_latch()
{
		pio_sm_exec(pio, sm, pio_encode_jmp(offset + tlc5952_write_offset_latch));
}

void disp_send_control()
{
	for (int i = num_chips; i > 0; i--)
	{
		uint32_t latch = (i == 1) ? 0x02'000000 : 0;
		pio_sm_put_blocking(pio, sm, control_buffer | latch);
	}
}

void disp_send_leds()
{
	for (const uint32_t on : onoff_buffer)
		pio_sm_put_blocking(pio, sm, on);
}

void disp_set_brightness(uint8_t bright)
{
	// The TLC5952 supplies less current to the blue channels,
	// so we need to slightly dim red and green to compensate.
	uint8_t brightRG = bright * 0.88f;  // Experimentally determined

	control_buffer = 0x01'000000 |
		bright   << 14 |  // Bright B 0-127
		brightRG <<  7 |  // Bright G 0-127
		brightRG;         // Bright R 0-127
}

void disp_set_digit(uint digit, uint8_t value, bool dp)
{
#if BOARD_V1
	static constexpr uint32_t dp_bits = 0x000001;
	static constexpr std::array<uint32_t, 10> digit_bits = {
		0x248248, // 0
		0x200008, // 1
		0x241240, // 2
		0x241048, // 3
		0x209008, // 4
		0x049048, // 5
		0x049248, // 6
		0x240008, // 7
		0x249248, // 8
		0x249048, // 9
	};

	uint chip   = num_chips - 1 - (digit / 3);
	uint offset = digit % 3;
	onoff_buffer[chip] &= ~((digit_bits[8] | dp_bits) << offset);
	onoff_buffer[chip] |= (digit_bits[value] | (dp ? dp_bits : 0)) << offset;
#else // BOARD_V2
	static constexpr uint32_t dp_bit = 0x000001;
	static constexpr std::array<uint8_t, 11> digit_bits = {
		0xEE, // 0
		0x82, // 1
		0xDC, // 2
		0xD6, // 3
		0xB2, // 4
		0x76, // 5
		0x7E, // 6
		0xC2, // 7
		0xFE, // 8
		0xF6, // 9
		0x00
	};

	uint chip   = num_chips - 1 - (digit / 3);
	uint offset = digit % 3;
	onoff_buffer[chip] &= ~((0xFF << (offset * 8)));
	onoff_buffer[chip] |= (digit_bits[value] | (dp ? dp_bit : 0)) << (offset * 8);
#endif
}

void disp_set_colons(bool on)
{
#if BOARD_V1
	constexpr uint32_t colon_bits = 0x000249;
#else // BOARD_V2
	constexpr uint32_t colon_bits = 0xFF;
#endif
	if (on)
		onoff_buffer[5] |= colon_bits;
	else
		onoff_buffer[5] &= ~colon_bits;
}

void disp_set_time(const time_split_t& time)
{
	disp_set_colons(true);
	disp_set_digit( 1,  time.year        / 1000 % 10, false);
	disp_set_digit( 2,  time.year        /  100 % 10, false);
	disp_set_digit( 3,  time.year        /   10 % 10, false);
	disp_set_digit( 4,  time.year               % 10, false);
	disp_set_digit( 5,  time.month       /   10 % 10, false);
	disp_set_digit( 6,  time.month              % 10, false);
	disp_set_digit( 7,  time.day         /   10 % 10, false);
	disp_set_digit( 8,  time.day                % 10, false);
	disp_set_digit( 9,  time.hour        /   10 % 10, false);
	disp_set_digit(10,  time.hour               % 10, false);
	disp_set_digit(11,  time.minute      /   10 % 10, false);
	disp_set_digit(12,  time.minute             % 10, false);
	disp_set_digit(13,  time.second      /   10 % 10, false);
	disp_set_digit(14,  time.second             % 10, true);
	disp_set_digit(15,  time.millisecond /  100 % 10, false);
	disp_set_digit(16,  time.millisecond /   10 % 10, false);
	disp_set_digit(17,  time.millisecond        % 10, false);
}
