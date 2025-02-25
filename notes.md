Pico W
======
https://www.raspberrypi.com/documentation/pico-sdk/
https://github.com/raspberrypi/pico-examples
https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf
3V3 recommended limit 300mA


GPS
===
https://www.adafruit.com/product/746#technical-details
https://cdn-shop.adafruit.com/product-files/746/CD+PA1616S+Datasheet.v03.pdf
Draws ~25mA
9600 N81

$GPRMC,041826.000,A,3959.0864,N,10514.5749,W,0.11,95.04,271124,,,A*4C
Recommended Minimum Navigation
    Time hhmmss.sss
    A=valid V=not 
    Latitude,  N/S            
    Longitude, W/E
    Speed knots
    Course degrees
    Date ddmmyy
    (Magnetic variation)
    A=Autonomous Mode
    Checksum


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

  YYYYMMDD    8 digits
HH:MM:SSsss   9 digits + 2 colons

24 channels/driver / 8 channels/digit = 3 digits/driver
17 digits / 3 digits/driver = 5+(16/24) drivers

$0.20 0.56" digits!  (50 pack, marketplace)
$0.30 0.80" digits!  (50 pack, marketplace, 15-day lead)
https://www.digikey.com/en/products/detail/luckylight/KW1-801AVB/22521896


Board revisions
===============
[ ] Level shifter on logic board
[ ] Alignment holes?
[ ] Battery?
[ ] Serial pins
[ ] GPS test points


Software TODO
=============
[x] Revisit sync
[x] DMA to PIO
[?] Interrupt update
[x] Flash settings
[x] Balance channel current
[x] Hook BLE into config better
[x] BLE command characteristic, to write flash
[ ] Startup screen; ID, test, ?
[x] Fix BLE time display
[c] BLE time accuracy
[x] BLE log
[ ] More BLE status?
[ ] Switch to nav message?


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