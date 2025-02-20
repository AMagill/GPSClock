#include "display.hpp"
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "tlc5952.pio.h"

static constexpr uint pio_sm    = 0;
static constexpr uint num_chips = 6;

static PIO  pio;
static uint pio_offset;
// 2 commands per chip; brightness and on/off
static std::array<uint32_t, num_chips*2> command_buffer;
static int dma_channel;

void disp_init(PIO pio, uint tx_pin, uint clk_pin, uint latch_pin)
{
	::pio = pio;

	gpio_init(latch_pin);
	gpio_set_dir(latch_pin, true);
	gpio_put(latch_pin, false);

	pio_offset = pio_add_program(pio, &tlc5952_write_program);
	tlc5952_write_program_init(pio, pio_sm, pio_offset, tx_pin, clk_pin, latch_pin);

	dma_channel = dma_claim_unused_channel(true);
	dma_channel_config dma_config = dma_channel_get_default_config(dma_channel);
	channel_config_set_dreq(&dma_config, pio_get_dreq(pio, pio_sm, true));  // Sending to PIO
	channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_32);  // 32 bits per word
	channel_config_set_read_increment(&dma_config, true);  // Do increment read address
	dma_channel_configure(dma_channel, &dma_config, 
		&pio->txf[pio_sm],  // Write address: PIO TX FIFO
		nullptr,            // No read address yet
		num_chips*2,        // Transfer count: 2 commands per chip
		false               // Don't trigger yet
	);
}

void disp_latch()
{
	pio_sm_exec(pio, pio_sm, pio_encode_jmp(pio_offset + tlc5952_write_offset_latch));
}

void disp_send()
{
	dma_channel_set_read_addr(dma_channel, command_buffer.data(), true);  // Start DMA transfer
}

void disp_set_brightness(uint8_t bright)
{
	// The TLC5952 supplies less current to the blue channels,
	// so we need to slightly dim red and green to compensate.
	uint8_t brightRG = bright * 0.88f;  // Experimentally determined

	for (int i = 0; i < num_chips; i++)
	{
		command_buffer[i] = 0x01'000000 |
			bright   << 14 |  // Bright B 0-127
			brightRG <<  7 |  // Bright G 0-127
			brightRG;         // Bright R 0-127
	}

	// Set flag to latch after the last chip
	command_buffer[num_chips-1] |= 0x02'000000;
}

void disp_set_digit(uint digit, uint8_t value, bool dp)
{
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
		0x00  // Blank
	};

	uint chip   = digit / 3;
	uint offset = (digit % 3) * 8;
	// +num_chips because the first half of the buffer is brightness settings
	command_buffer[num_chips + chip] &= ~(0xFF << offset);
	command_buffer[num_chips + chip] |= (digit_bits[value] | (dp ? dp_bit : 0)) << offset;
}

void disp_set_raw(uint chip, uint32_t value)
{
	command_buffer[num_chips + chip] = value & 0x00FFFFFF;
}

void disp_set_colons(bool on)
{
	constexpr uint32_t colon_bits = 0x0000FF;
	if (on)
		command_buffer[num_chips] |= colon_bits;
	else
		command_buffer[num_chips] &= ~colon_bits;
}

void disp_set_time(const Time_Parts& time, Time_Quality quality)
{
	disp_set_colons(true);

	if (quality == Time_Quality::INVALID)
	{	// Blank date when time is invalid; basically a timer since boot
		for (int i = 1; i <= 8; i++)
			disp_set_digit(i, 10, false);
	}
	else
	{
		disp_set_digit(1, time.year  / 1000 % 10, false);
		disp_set_digit(2, time.year  /  100 % 10, false);
		disp_set_digit(3, time.year  /   10 % 10, false);
		disp_set_digit(4, time.year         % 10, false);
		disp_set_digit(5, time.month /   10 % 10, false);
		disp_set_digit(6, time.month        % 10, false);
		disp_set_digit(7, time.day   /   10 % 10, false);
		disp_set_digit(8, time.day          % 10, false);
	}

	disp_set_digit( 9, time.hour   / 10 % 10, false);
	disp_set_digit(10, time.hour        % 10, false);
	disp_set_digit(11, time.minute / 10 % 10, false);
	disp_set_digit(12, time.minute      % 10, false);
	disp_set_digit(13, time.second / 10 % 10, false);
	disp_set_digit(14, time.second      % 10, true);

	// Degrade display resolution as quality decreases
	if (quality == Time_Quality::LOW)
	{
		disp_set_digit(15, time.millisecond / 100 % 10, false);
		disp_set_digit(16, 10, false);
		disp_set_digit(17, 10, false);
	}
	else if (quality == Time_Quality::MEDIUM)
	{	
		disp_set_digit(15, time.millisecond / 100 % 10, false);
		disp_set_digit(16, time.millisecond /  10 % 10, false);
		disp_set_digit(17, 10, false);
	}
	else if (quality == Time_Quality::HIGH || quality == Time_Quality::INVALID)
	{
		disp_set_digit(15, time.millisecond / 100 % 10, false);
		disp_set_digit(16, time.millisecond /  10 % 10, false);
		disp_set_digit(17, time.millisecond       % 10, false);
	}
}
