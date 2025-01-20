#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "gps.h"
#include "display.h"
#include "ble.h"
#include "config.h"

#define GPS_PPS_PIN 4

Config config;

void gpio_isr(uint gpio, uint32_t event_mask)
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
		using namespace std::chrono;
		time_us_t time_us = gps_get_time();
		auto date_days = floor<days>(time_us);
    year_month_day ymd{date_days};
    hh_mm_ss time_hms{floor<milliseconds>(time_us-date_days)};
    int year = static_cast<int>(ymd.year());
    int mon  = static_cast<unsigned int>(ymd.month());
    int day  = static_cast<unsigned int>(ymd.day());
    int hour = time_hms.hours()      / 1h;
    int min  = time_hms.minutes()    / 1min;
    int sec  = time_hms.seconds()    / 1s;
    int ms   = time_hms.subseconds() / 1ms;

		disp_set_brightness(ble_get_brightness());
		disp_set_colons(true);
		//disp_set_digit( 0,  0, false);
		disp_set_digit( 1,  year / 1000 % 10, false);
		disp_set_digit( 2,  year /  100 % 10, false);
		disp_set_digit( 3,  year /   10 % 10, false);
		disp_set_digit( 4,  year        % 10, false);
		disp_set_digit( 5,  mon  /   10 % 10, false);
		disp_set_digit( 6,  mon         % 10, false);
		disp_set_digit( 7,  day  /   10 % 10, false);
		disp_set_digit( 8,  day         % 10, false);
		disp_set_digit( 9,  hour /   10 % 10, false);
		disp_set_digit(10,  hour        % 10, false);
		disp_set_digit(11,  min  /   10 % 10, false);
		disp_set_digit(12,  min         % 10, false);
		disp_set_digit(13,  sec  /   10 % 10, false);
		disp_set_digit(14,  sec         % 10, true);
		disp_set_digit(15,  ms   /  100 % 10, false);
		disp_set_digit(16,  ms   /   10 % 10, false);
		disp_set_digit(17,  ms          % 10, false);

		disp_latch();
		disp_send_control();
		disp_send_leds();

		if (last_s != sec)
		{
			last_s = sec;
			ble_tick_time(year, mon, day, hour, min, sec);
		}

		sleep_ms(1);
	}
}
