#include "simple_midi.h"
#include <stdint.h>
#include <Arduino.h>

#define MIDI_AC_INTERFACE 	pluggedInterface	// MIDI AC Interface
#define MIDI_INTERFACE 		((uint8_t)(pluggedInterface+1))
#define MIDI_FIRST_ENDPOINT pluggedEndpoint
#define MIDI_ENDPOINT_IN	pluggedEndpoint

#define MIDI_TX MIDI_ENDPOINT_IN

SimpleMIDI MIDI;

SimpleMIDI::SimpleMIDI() : PluggableUSBModule(1, 2, epType)
{
	epType[0] = EP_TYPE_BULK_IN_MIDI; // MIDI_ENDPOINT_IN
	PluggableUSB().plug(this);
}

void SimpleMIDI::flush()
{
	USB_Flush(MIDI_TX);
}

size_t SimpleMIDI::write(const uint8_t *buffer, size_t size)
{
	int r = 0;
	if (is_write_enabled(MIDI_TX)) {
		r = USB_Send(MIDI_TX, buffer, size);
	}
	return r;
}

void SimpleMIDI::sendMIDI(midiEventPacket_t event)
{
	uint8_t data[4];
	data[0] = event.header;
	data[1] = event.byte1;
	data[2] = event.byte2;
	data[3] = event.byte3;
	write(data, 4);
}

//
//

#define MIDI_AUDIO								0x01
#define MIDI_AUDIO_CONTROL						0x01
#define MIDI_CS_INTERFACE						0x24
#define MIDI_CS_ENDPOINT						0x25
#define MIDI_STREAMING							0x3
#define MIDI_JACK_EMD							0x01
#define MIDI_JACK_EXT							0X02

_Pragma("pack(1)")
/// Midi Audio Control Interface Descriptor
typedef struct
{
	uint8_t len;		// 9
	uint8_t dtype;		// 4
	uint8_t dsubType;
	uint16_t bcdADc;
	uint16_t wTotalLength;
	uint8_t bInCollection;
	uint8_t interfaceNumbers;
} MIDI_ACInterfaceDescriptor;

typedef struct
{
	uint8_t len;		// 9
	uint8_t dtype;		// 4
	uint8_t dsubType;
	uint8_t jackType;
	uint8_t jackID;
	uint8_t nPins;
	uint8_t srcJackID;
	uint8_t srcPinID;
	uint8_t jackStrIndex;
} MIDIJackOutDescriptor;

/// MIDI Jack EndPoint Descriptor, common to midi in and out jacks.
typedef struct
{
	EndpointDescriptor len;		// 9
	uint8_t refresh;		// 4
	uint8_t sync;
} MIDI_EPDescriptor;

/// MIDI Jack  EndPoint AudioControl Descriptor, common to midi in and out ac jacks.
typedef struct
{
	uint8_t len;		// 5
	uint8_t dtype;		// 0x24
	uint8_t subtype;
	uint8_t embJacks;
	uint8_t jackID;
} MIDI_EP_ACDescriptor;

/// MIDI Audio Stream Descriptor Interface
typedef struct
{
	uint8_t len;		// 9
	uint8_t dtype;		// 4
	uint8_t dsubType;
	uint16_t bcdADc;
	uint16_t wTotalLength;
} MIDI_ASInterfaceDescriptor;

/// Top Level MIDI Descriptor used to create a Midi Interface instace \see MIDI_::getInterface()
typedef struct
{
	//	IAD
	IADDescriptor                      iad;
	// MIDI Audio Control Interface
	InterfaceDescriptor                Audio_ControlInterface;
	MIDI_ACInterfaceDescriptor         Audio_ControlInterface_SPC;

	// MIDI Audio Streaming Interface
	InterfaceDescriptor                Audio_StreamInterface;
	MIDI_ASInterfaceDescriptor         Audio_StreamInterface_SPC;

	MIDIJackOutDescriptor              MIDI_Out_Jack_Emb;
	MIDIJackOutDescriptor              MIDI_Out_Jack_Ext;
	MIDI_EPDescriptor                  MIDI_Out_Jack_Endpoint;
	MIDI_EP_ACDescriptor               MIDI_Out_Jack_Endpoint_SPC;
} MIDIDescriptor;

#define D_AC_INTERFACE(_streamingInterfaces, _MIDIInterface) \
	{ 9, MIDI_CS_INTERFACE, 0x1, 0x0100, 0x0009, _streamingInterfaces, (uint8_t)(_MIDIInterface) }

#define D_AS_INTERFACE \
	{ 0x7, MIDI_CS_INTERFACE, 0x01,0x0100, 0x0021}

#define D_MIDI_OUTJACK(jackProp, _jackID, _nPins, _srcID, _srcPin) \
	{ 0x09, MIDI_CS_INTERFACE, 0x3, jackProp, _jackID, _nPins, _srcID, _srcPin, 0  }

#define D_MIDI_JACK_EP(_addr,_attr,_packetSize) \
	{ 9, 5, _addr,_attr,_packetSize, 0, 0, 0}

#define D_MIDI_AC_JACK_EP(_nMIDI, _iDMIDI) \
	{ 5, MIDI_CS_ENDPOINT, 0x1, _nMIDI, _iDMIDI}

#define D_CDCCS(_subtype,_d0,_d1)	{ 5, 0x24, _subtype, _d0, _d1 }
#define D_CDCCS4(_subtype,_d0)		{ 4, 0x24, _subtype, _d0 }

#ifndef DOXYGEN_ARD
// the following would confuse doxygen documentation tool, so skip in that case for autodoc build
_Pragma("pack()")

#define WEAK __attribute__ ((weak))

#endif

int SimpleMIDI::getInterface(uint8_t* interfaceNum)
{
	interfaceNum[0] += 2;	// uses 2 interfaces
	MIDIDescriptor _midiInterface =
	{
		// TODO: check this is correct
		D_IAD(MIDI_AC_INTERFACE, 2, MIDI_AUDIO, MIDI_AUDIO_CONTROL, 0),
		D_INTERFACE(MIDI_AC_INTERFACE, 0, MIDI_AUDIO, MIDI_AUDIO_CONTROL, 0),
		D_AC_INTERFACE(0x1, MIDI_INTERFACE),
		D_INTERFACE(MIDI_INTERFACE, 1, MIDI_AUDIO, MIDI_STREAMING, 0),
		D_AS_INTERFACE,
		D_MIDI_OUTJACK(MIDI_JACK_EMD, 0x1, 1, 2, 1),
		D_MIDI_OUTJACK(MIDI_JACK_EXT, 0x2, 1, 1, 1),
		D_MIDI_JACK_EP(USB_ENDPOINT_IN(MIDI_ENDPOINT_IN),USB_ENDPOINT_TYPE_BULK,MIDI_BUFFER_SIZE),
		D_MIDI_AC_JACK_EP (1, 1)
	};
	return USB_SendControl(0, &_midiInterface, sizeof(_midiInterface));
}

int SimpleMIDI::getDescriptor(USBSetup& setup)
{
	return 0;
}

bool SimpleMIDI::setup(USBSetup& setup)
{
	return false;
}

uint8_t SimpleMIDI::getShortName(char* name)
{
	return 0;
}