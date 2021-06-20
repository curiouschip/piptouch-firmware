# TODO

[ ] Verify that MIDI descriptor format is correct

[ ] Compile/package bootloader, commit to Github (with binary), document pinout/changes
[ ] Create board package (inc. bootloader) + publish; update Arduino project to use it.

[ ] Automated tests for all functionality (inc. cli tools for setting/verifying cap touch settings)

[x] Reinstate recalibration

# Bootloader

Board designed to work with standard Leonardo (Caterina) bootloader.

Indication pins:

	- B0 - RX LED
	- C7 - Bootloader LED
	- D5 - TX LED
