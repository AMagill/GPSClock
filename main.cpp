#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/flash.h"
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
	uint64_t clock_offset_us = gps_get_clock_offset_us();
	uint32_t time_acc = gps_get_time_accuracy_ns();

	using namespace std::chrono;
	uint64_t hw_time = to_us_since_boot(get_absolute_time());
	Time_us time_us = Time_us(microseconds(clock_offset_us)) + microseconds(hw_time);

	if (clock_offset_us > 0)
		time_us += config.time_zone * 1h;
	// We're setting up for the next millisecond, so we can just latch it when it's time to display
	time_us += 1000us;
	Time_Parts time = time_split(time_us);

	// Update the display for the next millisecond
	disp_clear();
	disp_set_brightness(config.brightness);
	disp_set_colons(true);

	if (clock_offset_us > 0)
	{
		disp_set_num(1, time.year  / 1000 % 10, false);
		disp_set_num(2, time.year  /  100 % 10, false);
		disp_set_num(3, time.year  /   10 % 10, false);
		disp_set_num(4, time.year         % 10, false);
		disp_set_num(5, time.month /   10 % 10, false);
		disp_set_num(6, time.month        % 10, false);
		disp_set_num(7, time.day   /   10 % 10, false);
		disp_set_num(8, time.day          % 10, false);
	}

	disp_set_num( 9, time.hour   / 10 % 10, false);
	disp_set_num(10, time.hour        % 10, false);
	disp_set_num(11, time.minute / 10 % 10, false);
	disp_set_num(12, time.minute      % 10, false);
	disp_set_num(13, time.second / 10 % 10, false);
	disp_set_num(14, time.second      % 10, true);

	// Degrade display resolution as quality decreases
	if (clock_offset_us == 0 || time_acc < 100'000'000)  // 100ms
		disp_set_num(15, time.millisecond / 100 % 10, false);

	if (clock_offset_us == 0 || time_acc < 10'000'000)  // 10ms
		disp_set_num(16, time.millisecond /  10 % 10, false);
	
	if (clock_offset_us == 0 || time_acc < 1'000'000)  // 1ms
		disp_set_num(17, time.millisecond       % 10, false);

	// Send the display data.  Latches brightness, but not state
	disp_send(false);

	// Send the time to BLE every second
	if (hw_time - last_ble_tick > 1'000'000)
	{
		last_ble_tick = hw_time;
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
	disp_send(true);

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

	// Startup screen
	disp_set_brightness(config.brightness);
	disp_set_colons(true);
	for (int i = 1; i < 18; i++)
		disp_set_num(i, 8, true);
	disp_send(true);
	sleep_ms(500);

	disp_clear();
	disp_set_brightness(config.brightness);
	{
		uint8_t id[FLASH_UNIQUE_ID_SIZE_BYTES];
		flash_get_unique_id(id);
		disp_set_num(7, id[FLASH_UNIQUE_ID_SIZE_BYTES-1] >> 4,   false);
		disp_set_num(8, id[FLASH_UNIQUE_ID_SIZE_BYTES-1] & 0x0f, false);
	}
	disp_send(true);
	sleep_ms(1000);

	// Set up the display refresh timer
	alarm_pool_init_default();
	add_alarm_in_us(1000, do_every_ms, nullptr, true);

	while (true)
	{
		sleep_ms(1000);
	}
}
