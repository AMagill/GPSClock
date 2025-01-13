#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "gps.h"
#include "display.h"
#include "ble.h"

#define DISP_LATCH_PIN  9
#define DISP_CLK_PIN   10
#define DISP_TX_PIN    11

#define GPS_UART uart1
#define GPS_BAUD 9600
#define GPS_RX_PIN 5
#define GPS_PPS_PIN 4

Gps gps;
Display disp;

void gps_uart_rx_isr()
{
	while (uart_is_readable(GPS_UART))
	{
		char ch = uart_getc(GPS_UART);
		gps.on_rx(ch);
	}
}

void gpio_isr(uint gpio, uint32_t event_mask)
{
	gps.on_pps();
}

int main()
{
	stdio_init_all();

	disp.init(pio0, DISP_TX_PIN, DISP_CLK_PIN, DISP_LATCH_PIN);

	// Set up the GPS UART
	uart_init(GPS_UART, GPS_BAUD);
	gpio_set_function(GPS_RX_PIN, GPIO_FUNC_UART);
	uint gps_uart_irq = UART_IRQ_NUM(GPS_UART);
	irq_set_exclusive_handler(gps_uart_irq, gps_uart_rx_isr);
	irq_set_enabled(gps_uart_irq, true);
	uart_set_irq_enables(GPS_UART, true, false);

	// Set up the PPS pin
	gpio_set_irq_callback(gpio_isr);
	gpio_set_irq_enabled(GPS_PPS_PIN, GPIO_IRQ_EDGE_RISE, true);
	irq_set_enabled(IO_IRQ_BANK0, true);

	ble_init();

	int8_t last_s = -1;
	while (true)
	{
		using namespace std::chrono;
		auto dp = floor<days>(gps.time());
    year_month_day ymd{dp};
    hh_mm_ss time{floor<milliseconds>(gps.time()-dp)};
    int year = static_cast<int>(ymd.year());
    int mon  = static_cast<unsigned int>(ymd.month());
    int day  = static_cast<unsigned int>(ymd.day());
    int hour = time.hours()      / 1h;
    int min  = time.minutes()    / 1min;
    int sec  = time.seconds()    / 1s;
    int ms   = time.subseconds() / 1ms;

		disp.set_brightness(64);
		disp.set_colons(true);
		//disp.set_digit( 0,  0, false);
		disp.set_digit( 1,  year / 1000 % 10, false);
		disp.set_digit( 2,  year /  100 % 10, false);
		disp.set_digit( 3,  year /   10 % 10, false);
		disp.set_digit( 4,  year        % 10, false);
		disp.set_digit( 5,  mon  /   10 % 10, false);
		disp.set_digit( 6,  mon         % 10, false);
		disp.set_digit( 7,  day  /   10 % 10, false);
		disp.set_digit( 8,  day         % 10, false);
		disp.set_digit( 9,  hour /   10 % 10, false);
		disp.set_digit(10,  hour        % 10, false);
		disp.set_digit(11,  min  /   10 % 10, false);
		disp.set_digit(12,  min         % 10, false);
		disp.set_digit(13,  sec  /   10 % 10, false);
		disp.set_digit(14,  sec         % 10, true);
		disp.set_digit(15,  ms   /  100 % 10, false);
		disp.set_digit(16,  ms   /   10 % 10, false);
		disp.set_digit(17,  ms          % 10, false);

		disp.latch();
		disp.send_control(32);
		disp.send_leds();

		if (last_s != sec)
		{
			last_s = sec;
			ble_tick_time(year, mon, day, hour, min, sec);
		}

		sleep_ms(1);
	}
}
