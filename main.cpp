#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "gps.hpp"
#include "display.hpp"
#include "ble.hpp"
#include "config.hpp"
#include "time.hpp"

#define GPS_PPS_PIN 4

using namespace std::chrono_literals;

Config config;

static void gpio_isr(uint gpio, uint32_t event_mask)
{
	if (gpio == GPS_PPS_PIN)
		gps_on_pps();
}

static void ble_command(BLECommand command)
{
	switch (command)
	{
	case BLECommand::SAVE_SETTINGS:
		config_write_to_flash(config);
		break;
	}
}

int main()
{
	stdio_init_all();

	gps_init(uart1, 9600, 5, 4);
	disp_init(pio0, 11, 10, 9);
	ble_init();  // Can take almost a second!
	ble_set_command_cb(ble_command);
	config_read_from_flash(config);

	// Set up the PPS pin
	gpio_set_irq_callback(gpio_isr);
	gpio_set_irq_enabled(GPS_PPS_PIN, GPIO_IRQ_EDGE_RISE, true);
	irq_set_enabled(IO_IRQ_BANK0, true);

	int8_t last_s = -1;
	while (true)
	{
		Time_Quality quality = gps_get_time_quality();
		Time_us time_us = gps_get_time();
		if (quality != Time_Quality::INVALID)
			time_us += config.time_zone * 1h;
		Time_Parts time_parts = time_split(time_us);

		disp_set_brightness(config.brightness);
		disp_set_time(time_parts, quality);

		disp_send();

		if (last_s != time_parts.second)
		{
			last_s = time_parts.second;
			ble_tick_time(time_parts);
		}

		uint64_t us_to_next_ms = 1000 - (time_us.time_since_epoch() % 1ms).count();
		sleep_us(us_to_next_ms);

		disp_latch();
	}
}
