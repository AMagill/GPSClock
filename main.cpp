#include <stdio.h>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "gps.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9

#define GPS_UART    uart1
#define GPS_BAUD    9600
#define GPS_RX_PIN  5
#define GPS_PPS_PIN 4

Gps gps;

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
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

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
    
    while (true) {
        auto foo = gps.time();
        volatile std::string bar = fmt::format("{:%y%m%d %H%M%S}", foo);//gps.time());
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
