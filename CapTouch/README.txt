# TODO

  - verify that MIDI descriptor format is correct
  - package up bootloader on Github (+ document pinout/changes)
  - create board package + bootloader
  - test on Windows



# Bootloader

Board designed to work with standard Leonardo (Caterina) bootloader.

Indication pins:

	- B0 - RX LED
	- C7 - Bootloader LED
	- D5 - TX LED

# TODO

[x] MIDI - implement pitch bend scaling - 30 mins
[x] MIDI - configurable channel 
[x] Fix LED order for slider tracking - 15 mins
[x] Command to set LEDs - 1hr
[x] MIDI - configurable controller number
[x] Console command to change current mode + default mode (+ save default mode to EEPROM) - 15 mins
[x] Tidy up main code file (extract cap touch stuff to own file) - 20 mins
[x] Setting to control buttons report format

Implement calibrate/reset button - 30 mins

Register settings in Settings + use on init
Ability to set settings over console

Finish off console:
  - needs to be a distinction between current + saved settings (e.g. tracking)
  - console commands + settings for configuring cap touch registers - 2 hr
  - settings - persistence format + CRC; initialise correctly - 1 hr

Gamepad support