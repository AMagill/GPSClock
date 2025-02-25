#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "gps.hpp"
#include "display.hpp"
#include "ble.hpp"
#include "config.hpp"
#include "time.hpp"

#define GPS_PPS_PIN 3

using namespace std::chrono_literals;

Config config;
uint64_t last_ble_tick = 0;

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

int64_t do_every_ms(alarm_id_t id, void *user_data)
{
	// Latch the display state we prepped last time
	disp_latch();

	// Get the time from GPS
	uint32_t time_acc = gps_get_time_accuracy_ns();
	Time_us time_us = gps_get_time();
	if (time_acc < 0xFFFFFFFF)
		time_us += config.time_zone * 1h;
	// We're setting up for the next millisecond, so we can just latch it when it's time to display
	time_us += 1000us;
	Time_Parts time = time_split(time_us);

	// Update the display for the next millisecond
	disp_set_brightness(config.brightness);
	disp_set_colons(true);

	bool time_invalid = time_acc == 0xFFFFFFFF;
	if (time_invalid)
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
	if (time_invalid || time_acc < 100'000'000)  // 100ms
		disp_set_digit(15, time.millisecond / 100 % 10, false);
	else
		disp_set_digit(15, 10, false);

	if (time_invalid || time_acc < 10'000'000)  // 10ms
		disp_set_digit(16, time.millisecond /  10 % 10, false);
	else
		disp_set_digit(16, 10, false);
	
	if (time_invalid || time_acc < 1'000'000)  // 1ms
		disp_set_digit(17, time.millisecond       % 10, false);
	else
		disp_set_digit(17, 10, false);

	// Send the display data.  Latches brightness, but not state
	disp_send();

	// Send the time to BLE every second
	uint64_t now = to_us_since_boot(get_absolute_time());
	if (now - last_ble_tick > 1'000'000)
	{
		last_ble_tick = now;
		ble_tick_time(time, time_acc);
	}

	// Schedule the next update
	int64_t us_to_next_ms = 1000 - (time_us.time_since_epoch() % 1ms).count();
	// It takes ~640us to send the display data, so make sure the next one doesn't interrupt
	if (us_to_next_ms < 800)
		us_to_next_ms += 1000;
	return us_to_next_ms;
}

int main()
{
	stdio_init_all();

	// Do early to keep TX glitch small
	gps_init_io(uart1, 9600, 5, 4);

	// Get the screen blanked
	disp_init(pio0, 11, 10, 9);
	disp_set_brightness(0);
	disp_send();

	// Load config from flash
	config_read_from_flash(config);

	// This can take almost a second!
	ble_init();
	ble_set_command_cb(ble_command);

	// Send init commands to GPS now that it's had plenty of time to start up
	gps_init_comms();

	// Set up the PPS pin
	gpio_set_irq_callback(gpio_isr);
	gpio_set_irq_enabled(GPS_PPS_PIN, GPIO_IRQ_EDGE_RISE, true);
	irq_set_enabled(IO_IRQ_BANK0, true);

	// Set up the display refresh timer
	alarm_pool_init_default();
	add_alarm_in_us(1000, do_every_ms, nullptr, true);

	while (true)
	{
		sleep_ms(1000);
	}
}
