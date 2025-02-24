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

void disp_test()
{
		// Startup screen test
		disp_set_brightness(127);
		for (int i = 0; i < 6; ++i)
		{
			uint32_t bits = 0;
			for (int j = 0; j < 24; ++j)
			{
				bits = bits<<1 | 1;
				disp_set_raw(i, bits);
				disp_send();
				disp_latch();
				sleep_ms(20);
			}
		}
		for (int i = 5; i >= 0; --i)
		{
			uint32_t bits = 0x00FFFFFF;
			for (int j = 0; j < 24; ++j)
			{
				bits >>= 1;
				disp_set_raw(i, bits);
				disp_send();
				disp_latch();
				sleep_ms(20);
			}
		}	
}