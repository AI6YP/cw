# CW Key

[Morse code](https://en.wikipedia.org/wiki/Morse_code) computer input interface.

## Features

  * Autimatic timing
  * USB [HID](https://en.wikipedia.org/wiki/USB_human_interface_device_class) interface
  * [Makey Makey](https://github.com/sparkfun/MaKeyMaKey)
  * Browser JS
  * PTT key
  * Capacitive touch paddle ENIG (Electroless Nickel Immersion Gold) finish
  * https://hackaday.io/project/19129-tiny-cw-capacitive-touch-paddle

* Rotary encoder
    - speed
    - volume
    - VFO
    - [400pulse](https://www.aliexpress.com/item/Free-Shipping-1pcx-Incremental-optical-rotary-encoder-400-pulse/1212996005.html)
  * Remote VFO
  
  * http://www.instructables.com/id/USB-Volume-Control-and-Caps-Lock-LED-Simple-Cheap-/
  * https://github.com/mrdavidjwatts/USB-Volume-Control
  * https://github.com/NicoHood/HID
  * https://www.ebay.com/itm/362187355931


## Interface

### C232HM-DDHSL-0

Red     Vcc     3.3v
Orange  TCK     SWCLK
Yellow  TDI     (SWDIO -- 470)
Green   TDO     SWDIO
Brown   TMS
...
Black   GND     GND
