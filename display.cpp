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

void disp_send(bool latch)
{
	if (latch)
		command_buffer[num_chips*2-1] |= 0x02'000000;  // Set flag to latch after the last chip
	dma_channel_set_read_addr(dma_channel, command_buffer.data(), true);  // Start DMA transfer
}

void disp_set_brightness(uint8_t bright)
{
	// The TLC5952 supplies less current to the blue channels,
	// so we need to slightly dim red and green to compensate.
	uint8_t brightRG = bright * 0.88f;  // Experimentally determined
	bright   = std::clamp<uint8_t>(bright,   1, 127);
	brightRG = std::clamp<uint8_t>(brightRG, 1, 127);

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

void disp_clear()
{
	for (int i = num_chips; i < num_chips*2; i++)
		command_buffer[i] = 0;
}

void disp_set_num(uint digit, uint8_t value, bool dp)
{
	static constexpr uint32_t dp_bit = 0x000001;
	static constexpr std::array<uint8_t, 16> digit_bits = {
		0xEE, // 0
		0x82, // 1
		0xDC, // 2       _40_
		0xD6, // 3   20 |    | 80
		0xB2, // 4      |_10_|
		0x76, // 5   08 |    | 02
		0x7E, // 6      |_04_|   O 01
		0xC2, // 7
		0xFE, // 8
		0xF6, // 9
		0xFA, // A
		0x3E, // b
		0x1C, // c
		0x9E, // d
		0x7C, // E
		0x78, // F
	};

	uint8_t leds = digit_bits[value] | (dp ? dp_bit : 0);
	disp_set_raw(digit, leds);
}

void disp_set_raw(uint digit, uint8_t value)
{
	uint chip   = digit / 3;
	uint offset = (digit % 3) * 8;
	// +num_chips because the first half of the buffer is brightness settings
	command_buffer[num_chips + chip] |= value << offset;
}

void disp_set_colons(bool on)
{
	constexpr uint32_t colon_bits = 0x0000FF;
	if (on)
		command_buffer[num_chips] |= colon_bits;
	else
		command_buffer[num_chips] &= ~colon_bits;
}
