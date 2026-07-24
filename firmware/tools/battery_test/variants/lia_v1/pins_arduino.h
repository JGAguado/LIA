#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

// Espressif's generic default -- LIA does not have its own registered USB VID/PID.
#define USB_VID 0x303a
#define USB_PID 0x1001

// Reserved, not connected on this board (see firmware/AGENTS.md).
static const uint8_t TX = 43;
static const uint8_t RX = 44;

// Actually used by this test: Wire.begin(SDA, SCL) is called explicitly.
static const uint8_t SDA = 4;
static const uint8_t SCL = 5;

static const uint8_t SS = 10;
static const uint8_t MOSI = 11;
static const uint8_t SCK = 12;
static const uint8_t MISO = 13;

#endif /* Pins_Arduino_h */
