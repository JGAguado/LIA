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

// Actually used on this revision: variant.h defines I2C_SDA/I2C_SCL (unlike
// lia_v1, where the IMU's presence made the bus unsafe to enable) so
// Meshtastic's stock Wire.begin(I2C_SDA, I2C_SCL) call picks these up.
static const uint8_t SDA = 4;
static const uint8_t SCL = 5;

static const uint8_t SS = 10;
static const uint8_t MOSI = 11;
static const uint8_t SCK = 12;
static const uint8_t MISO = 13;

#endif /* Pins_Arduino_h */
