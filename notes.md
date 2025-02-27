Pico W
======
https://www.raspberrypi.com/documentation/pico-sdk/
https://github.com/raspberrypi/pico-examples
https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf
3V3 recommended limit 300mA


GPS
===
uBlox CAM-M8Q-0
https://www.digikey.com/en/products/detail/u-blox/CAM-M8Q-0/6150646
https://content.u-blox.com/sites/default/files/CAM-M8-FW3_DataSheet_%28UBX-15031574%29.pdf


Display
=======
TLC5952 LED driver
https://www.digikey.com/en/products/detail/texas-instruments/TLC5952DAP/2078405
https://www.ti.com/lit/ds/symlink/tlc5952.pdf
24 channels, 1 bit per channel
3 groups, 7-bit dimming per group
35MHz data rate
Connects to cathodes, so get common-anode digits.
$2.68

YYYY MM DD    8 digits
HH:MM:SSsss   9 digits + 2 colons

24 channels/driver / 8 channels/digit = 3 digits/driver
17 digits / 3 digits/driver = 5+(16/24) drivers

KW1-801AVB 7-segment LEDs
https://www.digikey.com/en/products/detail/luckylight/KW1-801AVB/22521896
$0.30 0.80" digits!  (50 pack, marketplace, 15-day lead)



Bluetooth
=========
https://bluekitchen-gmbh.com/btstack/v1.0/profiles/#gatt-generic-attribute-profile
https://github.dev/raspberrypi/pico-examples/blob/master/pico_w/bt/standalone/server.c

https://github.dev/bluekitchen/btstack/blob/master/example/le_streamer_client.c

https://github.dev/loginov-rocks/Web-Bluetooth-Terminal
https://loginov-rocks.github.io/Web-Bluetooth-Terminal/

https://developer.mozilla.org/en-US/docs/Web/API/Bluetooth/requestDevice


Flash
=====
https://www.makermatrix.com/blog/read-and-write-data-with-the-pi-pico-onboard-flash/
https://kevinboone.me/picoflash.html