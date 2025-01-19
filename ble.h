#include <cstdint>
#include <string>

void ble_init();
void ble_tick_time(int year, int month, int day, int hour, int min, int sec);
uint8_t ble_get_brightness();
void    ble_set_brightness(uint8_t bright);