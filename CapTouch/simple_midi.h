#ifndef SIMPLE_MIDI_H
#define SIMPLE_MIDI_H

#include "PluggableUSB.h"
#include "usb_defs.h"

#include <stdint.h>

typedef struct
{
	uint8_t header;
	uint8_t byte1;
	uint8_t byte2;
	uint8_t byte3;
} midiEventPacket_t;

class SimpleMIDI : public PluggableUSBModule
{
public:
	SimpleMIDI();
	void flush();
	size_t write(const uint8_t *buffer, size_t size);
	void sendMIDI(midiEventPacket_t event);

protected:
	int getInterface(uint8_t* interfaceNum);
	int getDescriptor(USBSetup& setup);
	bool setup(USBSetup& setup);
	uint8_t getShortName(char* name);

private:
	EPTYPE_DESCRIPTOR_SIZE epType[1];
};

extern SimpleMIDI MIDI;

#endif