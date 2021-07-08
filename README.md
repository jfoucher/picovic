# PICOVIC

This is a Commodore Vic-20 emulator for the [Raspberry Pi Pico](https://www.raspberrypi.org/products/raspberry-pi-pico/)

Most of the 6502 emulation code is from [this assembly code](https://github.com/dp111/PicoTube/blob/master/copro-65tubeasmM0.S) with some modifications to support this use case.

At the moment, the colors are all wrong and you can only interact with it via the serial terminal.

The emulation is extremely basic and for example the are no emulated vias and only basic VIC chip functionality, in the interest of speed and laziness. However since for the time being the emulated speed is higher than the original VIC-20, more functionality will be added in the future.

