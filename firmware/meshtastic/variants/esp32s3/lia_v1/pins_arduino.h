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

// Arduino-ESP32's Wire library references SDA/SCL unconditionally at compile
// time (TwoWire::initPins()'s fallback default), regardless of whether
// anything actually calls Wire.begin(). Declaring the pin numbers here does
// NOT initialize or scan the I2C bus -- only an explicit Wire.begin() call
// would, and neither Meshtastic core nor our own code makes one without the
// I2C_SDA/I2C_SCL variant.h macros, which are deliberately not defined (see
// firmware/AGENTS.md "Hardware Constraints").
static const uint8_t SDA = 4;
static const uint8_t SCL = 5;

static const uint8_t SS = 10;
static const uint8_t MOSI = 11;
static const uint8_t SCK = 12;
static const uint8_t MISO = 13;

#endif /* Pins_Arduino_h */
