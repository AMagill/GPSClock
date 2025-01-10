#include <cstdint>
#include <array>
#include <string_view>
#include "hardware/pio.h"

class Display
{
public:
	void init(PIO pio, uint tx_pin, uint clk_pin, uint latch_pin);
	void latch();
	void send_control(uint8_t bright);
	void send_leds();

	void set_brightness(uint8_t bright);
	void set_digit(uint digit, uint8_t value, bool dp);
	void set_colons(bool on);

private:
	static constexpr uint sm = 0;
	static constexpr uint num_chips = 6;

	PIO m_pio;
	uint m_latch_pin;
	uint m_offset;
	uint32_t m_control_buffer;
	std::array<uint32_t, num_chips> m_onoff_buffer;
};