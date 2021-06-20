#ifndef USB_DEFS_H
#define USB_DEFS_H

#if defined(ARDUINO_ARCH_AVR)

#include "PluggableUSB.h"

#define EPTYPE_DESCRIPTOR_SIZE		uint8_t
#define EP_TYPE_BULK_IN_MIDI 		EP_TYPE_BULK_IN
#define EP_TYPE_BULK_OUT_MIDI 		EP_TYPE_BULK_OUT
#define MIDI_BUFFER_SIZE			USB_EP_SIZE
#define is_write_enabled(x)			(1)

#elif defined(ARDUINO_ARCH_SAM)

#include "USB/PluggableUSB.h"

#define EPTYPE_DESCRIPTOR_SIZE		uint32_t
#define EP_TYPE_BULK_IN_MIDI		(UOTGHS_DEVEPTCFG_EPSIZE_512_BYTE | \
									UOTGHS_DEVEPTCFG_EPDIR_IN |         \
									UOTGHS_DEVEPTCFG_EPTYPE_BLK |       \
									UOTGHS_DEVEPTCFG_EPBK_1_BANK |      \
									UOTGHS_DEVEPTCFG_NBTRANS_1_TRANS |  \
									UOTGHS_DEVEPTCFG_ALLOC)
#define EP_TYPE_BULK_OUT_MIDI       (UOTGHS_DEVEPTCFG_EPSIZE_512_BYTE | \
									UOTGHS_DEVEPTCFG_EPTYPE_BLK |       \
									UOTGHS_DEVEPTCFG_EPBK_1_BANK |      \
									UOTGHS_DEVEPTCFG_NBTRANS_1_TRANS |  \
									UOTGHS_DEVEPTCFG_ALLOC)
#define MIDI_BUFFER_SIZE			EPX_SIZE
#define USB_SendControl				USBD_SendControl
#define USB_Available				USBD_Available
#define USB_Recv					USBD_Recv
#define USB_Send					USBD_Send
#define USB_Flush					USBD_Flush
#define is_write_enabled(x)			Is_udd_write_enabled(x)

#define USB_EP_SIZE 				64
#define TRANSFER_PGM 				0x80

#elif defined(ARDUINO_ARCH_SAMD)

#if defined(ARDUINO_API_VERSION)
#include "api/PluggableUSB.h"
#define EPTYPE_DESCRIPTOR_SIZE		unsigned int
#else
#include "USB/PluggableUSB.h"
#define EPTYPE_DESCRIPTOR_SIZE		uint32_t
#endif
#define EP_TYPE_BULK_IN_MIDI 		USB_ENDPOINT_TYPE_BULK | USB_ENDPOINT_IN(0);
#define EP_TYPE_BULK_OUT_MIDI 		USB_ENDPOINT_TYPE_BULK | USB_ENDPOINT_OUT(0);
#define MIDI_BUFFER_SIZE			EPX_SIZE
#define USB_SendControl				USBDevice.sendControl
#define USB_Available				USBDevice.available
#define USB_Recv					USBDevice.recv
#define USB_Send					USBDevice.send
#define USB_Flush					USBDevice.flush
#define is_write_enabled(x)			(1)

#else

#error "Unsupported architecture"

#endif

#endif