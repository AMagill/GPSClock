#include "ble.hpp"
#include "config.hpp"
#include "btstack.h"
#include "btstack_run_loop_embedded.h"
#include "hci_dump_embedded_stdout.h"
#include "hci_transport_h4.h"
#include "pico/cyw43_arch.h"
#include "pico/btstack_cyw43.h"
#include "ble_config.h"
#include "hardware/flash.h"
#include <format>

#define CH_COMMAND       ATT_CHARACTERISTIC_00000002_B0A0_475D_A2F4_A32CD026A911_01_VALUE_HANDLE
#define CH_TIME          ATT_CHARACTERISTIC_00000003_B0A0_475D_A2F4_A32CD026A911_01_VALUE_HANDLE
#define CH_TIME_CFG      ATT_CHARACTERISTIC_00000003_B0A0_475D_A2F4_A32CD026A911_01_CLIENT_CONFIGURATION_HANDLE
#define CH_TIME_ZONE     ATT_CHARACTERISTIC_00000004_B0A0_475D_A2F4_A32CD026A911_01_VALUE_HANDLE
#define CH_BRIGHT        ATT_CHARACTERISTIC_00000005_B0A0_475D_A2F4_A32CD026A911_01_VALUE_HANDLE
#define CH_TIME_ACC      ATT_CHARACTERISTIC_00000006_B0A0_475D_A2F4_A32CD026A911_01_VALUE_HANDLE

extern Config config;

extern uint8_t const profile_data[];  // From the GATT header file
static btstack_packet_callback_registration_t hci_event_callback_registration;

static uint8_t adv_data[] = {
    // Flags general discoverable
     2, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,
    // Name
    13, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'G', 'P', 'S', ' ', 'C', 'l', 'o', 'c', 'k', ' ', '0', '0',
};
static_assert(sizeof(adv_data) <= 31, "adv_data too long");  // BLE limitation

static uint16_t time_client_config;
static hci_con_handle_t con_handle;
static uint8_t current_time[19] = {0};  // "YYYY-MM-DD hh:mm:ss"
static uint32_t time_acc = 0xFFFFFFFF;
static std::function<void(BLECommand)> command_cb;

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) 
{
	UNUSED(size);
	UNUSED(channel);
	bd_addr_t local_addr;
	if (packet_type != HCI_EVENT_PACKET) return;

	uint8_t event_type = hci_event_packet_get_type(packet);
	switch(event_type)
	{
	case BTSTACK_EVENT_STATE:
	{
		if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
		gap_local_bd_addr(local_addr);
		printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));

		// setup advertisements
		uint16_t adv_int_min = 800;
		uint16_t adv_int_max = 800;
		uint8_t adv_type = 0;
		bd_addr_t null_addr;
		memset(null_addr, 0, 6);
		
		{ // Put a unique byte in the advertised name
			uint8_t id[FLASH_UNIQUE_ID_SIZE_BYTES];
			flash_get_unique_id(id);
			assert(adv_data[15] == '0' && adv_data[16] == '0');
			adv_data[15] = char_for_nibble(id[FLASH_UNIQUE_ID_SIZE_BYTES-1] >> 4);
			adv_data[16] = char_for_nibble(id[FLASH_UNIQUE_ID_SIZE_BYTES-1] & 0x0f);
		}
		
		gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
		gap_advertisements_set_data(sizeof(adv_data), adv_data);
		gap_advertisements_enable(1);
		break;
	}
	case HCI_EVENT_DISCONNECTION_COMPLETE:
		time_client_config = 0;
		break;
	case ATT_EVENT_CAN_SEND_NOW:
		att_server_notify(con_handle, CH_TIME,     current_time,        sizeof(current_time));
		att_server_notify(con_handle, CH_TIME_ACC, (uint8_t*)&time_acc, sizeof(time_acc));
		break;
	default:
		break;
	}
}

static uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, 
	uint16_t offset, uint8_t* buffer, uint16_t buffer_size) 
{
		UNUSED(connection_handle);
		
		switch (att_handle) 
		{
		case CH_TIME:
			return att_read_callback_handle_blob(current_time, sizeof(current_time), offset, buffer, buffer_size);
		case CH_TIME_CFG:
			return att_read_callback_handle_little_endian_16(time_client_config, offset, buffer, buffer_size);
		case CH_TIME_ZONE:
			return att_read_callback_handle_little_endian_32(config.time_zone, offset, buffer, buffer_size);
		case CH_BRIGHT:
			return att_read_callback_handle_byte(config.brightness, offset, buffer, buffer_size);
		}

		return 0;
}

static int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, 
	uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) 
{
		UNUSED(transaction_mode);
		UNUSED(offset);
		
		switch (att_handle) 
		{
		case CH_COMMAND:
			if (command_cb)
			{
				uint32_t command = little_endian_read_32(buffer, 0);
				switch (command)  // Values are random 32-bit ints
				{
				case 0x31a86b97:  // Save settings
					command_cb(BLECommand::SAVE_SETTINGS);
					break;
				}
			}
			break;
		case CH_TIME_CFG:
			time_client_config = little_endian_read_16(buffer, 0);
			con_handle = connection_handle;
			break;
		case CH_TIME_ZONE:
			config.time_zone = little_endian_read_32(buffer, 0);
			break;
		case CH_BRIGHT:
			config.brightness = buffer[0];
			break;
		}

		return 0;
}

void ble_init()
{
	if (cyw43_arch_init()) {
			printf("failed to initialise cyw43_arch\n");
	}

	l2cap_init();
	sm_init();

	att_server_init(profile_data, att_read_callback, att_write_callback);    

	// inform about BTstack state
	hci_event_callback_registration.callback = &packet_handler;
	hci_add_event_handler(&hci_event_callback_registration);

	// register for ATT event
	att_server_register_packet_handler(packet_handler);

	// turn on bluetooth!
	hci_power_control(HCI_POWER_ON);
}

void ble_tick_time(const Time_Parts& time, uint32_t time_acc)
{
	std::format_to_n(current_time, sizeof(current_time), 
		"{:04}-{:02}-{:02} {:02}:{:02}:{:02}", 
		time.year, time.month, time.day, 
		time.hour, time.minute, time.second);
	::time_acc = time_acc;
	if (time_client_config & GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_INDICATION)
		att_server_request_can_send_now_event(con_handle);
}

void ble_set_command_cb(std::function<void(BLECommand)> cb)
{
	command_cb = cb;
}
