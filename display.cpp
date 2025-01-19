#include "display.h"
#include "pico/stdlib.h"
#include "tlc5952.pio.h"

void Display::init(PIO pio, uint tx_pin, uint clk_pin, uint latch_pin)
{
	m_pio = pio;
	m_latch_pin = latch_pin;

	gpio_init(latch_pin);
	gpio_set_dir(latch_pin, true);
	gpio_put(latch_pin, false);

	m_offset = pio_add_program(pio, &tlc5952_write_program);
	tlc5952_write_program_init(pio, sm, m_offset, tx_pin, clk_pin, latch_pin);
}

void Display::latch()
{
		pio_sm_exec(m_pio, sm, pio_encode_jmp(m_offset + tlc5952_write_offset_latch));
}

void Display::send_control(uint8_t bright)
{
	for (int i = num_chips; i > 0; i--)
	{
		uint32_t latch = (i == 1) ? 0x02'000000 : 0;
		pio_sm_put_blocking(m_pio, sm, m_control_buffer | latch);
	}
}

void Display::send_leds()
{
	for (const uint32_t on : m_onoff_buffer)
		pio_sm_put_blocking(m_pio, sm, on);
}

void Display::set_brightness(uint8_t bright)
{
	// The TLC5952 supplies less current to the blue channels,
	// so we need to adjust the brightness to compensate.
	uint8_t brightRG = bright * 0.88;

	m_control_buffer = 0x01'000000 |
		bright   << 14 |  // Bright B 0-127
		brightRG <<  7 |  // Bright G 0-127
		brightRG;         // Bright R 0-127
}

void Display::set_digit(uint digit, uint8_t value, bool dp)
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
	m_onoff_buffer[chip] &= ~((digit_bits[8] | dp_bits) << offset);
	m_onoff_buffer[chip] |= (digit_bits[value] | (dp ? dp_bits : 0)) << offset;
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
	m_onoff_buffer[chip] &= ~((0xFF << (offset * 8)));
	m_onoff_buffer[chip] |= (digit_bits[value] | (dp ? dp_bit : 0)) << (offset * 8);
#endif
}

void Display::set_colons(bool on)
{
#if BOARD_V1
	constexpr uint32_t colon_bits = 0x000249;
#else // BOARD_V2
	constexpr uint32_t colon_bits = 0xFF;
#endif
	if (on)
		m_onoff_buffer[5] |= colon_bits;
	else
		m_onoff_buffer[5] &= ~colon_bits;
}
