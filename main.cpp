#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "gps.hpp"
#include "display.hpp"
#include "ble.hpp"
#include "config.hpp"
#include "time.hpp"

#define GPS_PPS_PIN 4

Config config;

static void gpio_isr(uint gpio, uint32_t event_mask)
{
	if (gpio == GPS_PPS_PIN)
		gps_on_pps();
}

int main()
{
	stdio_init_all();

	disp_init(pio0, 11, 10, 9);
	gps_init(uart1, 9600, 5);

	// Set up the PPS pin
	gpio_set_irq_callback(gpio_isr);
	gpio_set_irq_enabled(GPS_PPS_PIN, GPIO_IRQ_EDGE_RISE, true);
	irq_set_enabled(IO_IRQ_BANK0, true);

	// Read configuration from flash
	config_read_from_flash(config);

	ble_init();
	ble_set_brightness(config.brightness);

	int8_t last_s = -1;
	while (true)
	{
		time_us_t time_us = gps_get_time();
		time_split_t time_parts = time_split(time_us);

		disp_set_brightness(ble_get_brightness());
		disp_set_time(time_parts);

		disp_latch();
		disp_send_control();
		disp_send_leds();

		if (last_s != time_parts.second)
		{
			last_s = time_parts.second;
			ble_tick_time(time_parts);
		}

		sleep_ms(1);
	}
}
