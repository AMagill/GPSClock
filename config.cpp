#include "config.hpp"
#include <boards/pico_w.h>
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <utility>
#include <algorithm>
#include <cstring>

extern uint32_t* __flash_binary_end;

// Let's use the very last sector of flash memory for configuration storage
static const uint32_t config_flash_start = PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE;
static uint8_t* const config_flash_addr  = (uint8_t*)config_flash_start + XIP_BASE;

struct Record
{
	uint32_t magic;
	Config   config;
};

static const int num_pages = FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE;
static const int records_per_page = FLASH_PAGE_SIZE / sizeof(Record);

static Record* find_last_record()
{
	Record* last_record = nullptr;
	for (int ipage = 0; ipage < num_pages; ipage++)
	{
		Record *page_records = (Record*)(config_flash_addr + ipage * FLASH_PAGE_SIZE);
		for (int irecord = 0; irecord < records_per_page; irecord++)
		{
			if (page_records[irecord].magic == Config::magic)
				last_record = &page_records[irecord];  // Looks like a valid record, remember it
			else
				return last_record;  // No more valid records
		}
	}
	return last_record;
}

void config_read_from_flash(Config &config)
{
	Record* last_record = find_last_record();

	// Fill the configuration
	if (last_record)
		config = last_record->config;
	else
	{
		// Default values
		config.time_zone  = 0;
		config.brightness = 64;
	}
}

static std::pair<int, int> find_empty_slot()
{
	// Find the first empty space for a new record
	// Iterating by page and record because we want to avoid records 
	// straddling pages and requiring two programs to write.
	for (int ipage = 0; ipage < num_pages; ipage++)
	{
		Record *page_records = (Record*)(config_flash_addr + ipage * FLASH_PAGE_SIZE);
		for (int irecord = 0; irecord < records_per_page; irecord++)
		{
			Record* record = &page_records[irecord];
			if (record->magic == Config::magic)
				continue;  // Looks like a valid record, skip over it
			else if (std::all_of((uint8_t*)record, (uint8_t*)record + sizeof(Record), [](uint8_t byte) { return byte == 0xff; }))
			{	// Found an empty slot
				return {ipage, irecord};
			}
			else
			{ // Not valid, and not empty.  We'd better erase and start fresh.
				return {-1, -1};
			}
		}
	}

	// No empty slot found
	return {-1, -1};
}

void config_write_to_flash(const Config &config)
{	
	auto [ipage, irecord] = find_empty_slot();

	// If we didn't find an empty slot, erase the whole sector.
	// This just resets all bits to one.  Programming turns them to zero.
	if (ipage == -1)
	{
		flash_range_erase(config_flash_start, FLASH_SECTOR_SIZE);
		ipage = 0;
		irecord = 0;
	}
	
	// No interrupts during flashing
	uint32_t ints = save_and_disable_interrupts();

	// Write the record.  We have to write a whole page at a time,
	// but only the zero bits will have any effect.  So we can just
	// write a page of ones except for the record we want to write.
	uint8_t page_buffer[FLASH_PAGE_SIZE];
	memset(page_buffer, 0xff, sizeof(page_buffer));
	Record* record = (Record*)(page_buffer + irecord * sizeof(Record));
	record->magic  = Config::magic;
	record->config = config;
	flash_range_program(config_flash_start + ipage * FLASH_PAGE_SIZE, page_buffer, FLASH_PAGE_SIZE);

	restore_interrupts(ints);
}
