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
[x] Make segment holes slightly bigger
[ ] Segments slightly closer together?
[x] Make USB port pads longer for soldering
[x] Pico debug pin hole kind of sucks
[ ] Tent vias
[x] Weird squiggle
[x] Reorder LEDs

Software TODO
=============
[ ] Revisit sync
[ ] DMA to PIO
