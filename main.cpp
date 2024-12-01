#include <stdio.h>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "gps.h"
#include "display.h"

#define DISP_PORT i2c1
#define DISP_SDA 26
#define DISP_SCL 27

#define GPS_UART uart1
#define GPS_BAUD 9600
#define GPS_RX_PIN 5
#define GPS_PPS_PIN 4

Gps gps;
std::array<Display, 4> disp;

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

	// I2C Initialisation. Using it at 400Khz.
	i2c_init(DISP_PORT, 400'000);
	gpio_set_function(DISP_SDA, GPIO_FUNC_I2C);
	gpio_set_function(DISP_SCL, GPIO_FUNC_I2C);
	gpio_pull_up(DISP_SDA);
	gpio_pull_up(DISP_SCL);

	disp[0].init(DISP_PORT, 0x70);
	disp[1].init(DISP_PORT, 0x71);
	disp[2].init(DISP_PORT, 0x72);
	disp[3].init(DISP_PORT, 0x73);

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

	while (true)
	{
		std::string bar = fmt::format("{:%y%m%d %H%M%S}", gps.time());

		std::string_view sv = bar;
		disp[0].set_disp(sv.substr(0, 4));
		disp[0].set_dots(false, true, false, true);
		disp[1].set_disp(sv.substr(4, 4));
		disp[2].set_disp(sv.substr(8, 4));
		disp[2].set_dots(true, false, true, false);
		disp[3].set_char(0, sv[12], true);
		disp[3].set_char(1, sv[14], false);
		disp[3].set_char(2, sv[15], false);
		disp[3].set_char(3, sv[16], false);

		disp[0].write_display();
		disp[1].write_display();
		disp[2].write_display();
		disp[3].write_display();

		sleep_ms(10);
	}
}
