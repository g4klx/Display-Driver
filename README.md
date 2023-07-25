This program allows displays to be used with MQTT equipped versions of the MMDVMHost. It takes
the structured information, in JSON format, provided by the host and converts it for use by
displays. It currently supports the display types that were in the MMDVMHost, but making this
handling external and simpler should allow for easier modifications and the addition of new display
types.

It builds on 32-bit and 64-bit Linux. It can control various displays, these are:

- HD44780 (sizes 2x16, 2x40, 4x16, 4x20)
	- Support for HD44780 via 4 bit GPIO connection (user selectable pins)
	- Adafruit 16x2 LCD+Keypad Kits (I2C)
	- Connection via PCF8574 GPIO Extender (I2C)
- Nextion TFTs (all sizes, both Basic and Enhanced versions)
- OLED 128x64 (SSD1306)
- LCDproc

The Nextion displays can connect to the UART on the Raspberry Pi, or via a USB
to TTL serial converter like the FT-232RL. It may also be connected to the UART
output of the MMDVM modem (Arduino Due, STM32, Teensy).

The HD44780 displays are integrated with wiringPi for Raspberry Pi based
platforms.

The OLED display needs an extra library see OLED.md

The LCDproc support enables the use of a multitude of other LCD screens. See
the [supported devices](http://lcdproc.omnipotent.net/hardware.php3) page on
the LCDproc website for more info.

This software is licenced under the GPL v2 and is primarily intended for amateur and
educational use.
