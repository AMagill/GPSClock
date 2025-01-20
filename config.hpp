#pragma once
#include <cstdint>

struct Config
{
	static const uint32_t magic = 0xddccc2fe;  // Random
	// If you change this struct, you must also change the magic!
	int32_t time_zone;
	uint8_t brightness;
};

void config_read_from_flash(Config& config);
void config_write_to_flash(const Config& config);