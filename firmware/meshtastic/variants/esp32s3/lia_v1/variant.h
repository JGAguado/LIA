// LIA v1 board variant -- ESP32-S3-MINI-1-N8.
//
// Pin assignments are silicon GPIO numbers, verified against the
// ESP32-S3-MINI-1-N8 (U1) symbol pin definitions and net labels in
// hardware/kicad/LIA.kicad_sch. See firmware/AGENTS.md for the authoritative
// pinout table and hardware constraints.
//
// Hardware constraint (current PCB revision): the I2C bus (SDA/SCL, IMU_INT)
// and the green/blue LED channels are NOT wired for use and MUST NOT be
// referenced here -- do not add I2C_SDA/I2C_SCL/HAS_RGB_LED/RGBLED_* defines.

#define _VARIANT_LIA_V1
#define LIA_V1

// -----------------------------------------------------------------------------
// SX1262 (Wio-SX1262 module)
// -----------------------------------------------------------------------------
#define USE_SX1262

#define LORA_SCK 12
#define LORA_MISO 13
#define LORA_MOSI 11
#define LORA_CS 10
#define LORA_RESET 8
#define LORA_DIO1 14

#define SX126X_CS LORA_CS
#define SX126X_DIO1 LORA_DIO1
#define SX126X_BUSY 9
#define SX126X_RESET LORA_RESET

// Wio-SX1262 module reference design: DIO2 drives the module's own internal
// antenna switch (SPI-configured on the SX1262 itself) and DIO3 enables its
// onboard 1.8V TCXO. Neither is broken out to the ESP32-S3 on LIA's schematic
// -- confirm both assumptions on first radio bring-up (Phase 2).
#define SX126X_DIO2_AS_RF_SWITCH
#define SX126X_DIO3_TCXO_VOLTAGE 1.8

// -----------------------------------------------------------------------------
// GNSS (u-blox SAM-M10Q)
// -----------------------------------------------------------------------------
#define HAS_GPS 1
#undef GPS_RX_PIN
#undef GPS_TX_PIN
#define GPS_TX_PIN 17 // ESP32-S3 -> GNSS RX
#define GPS_RX_PIN 18 // ESP32-S3 <- GNSS TX
#define GPS_BAUDRATE 9600

// -----------------------------------------------------------------------------
// LIA-specific pins, owned exclusively by LiaBoard (firmware/board/LiaBoard.*)
// -- not stock Meshtastic features, so no upstream macro exists for them.
// -----------------------------------------------------------------------------

// Peripheral power control. MUST be HIGH before the SX1262 or SAM-M10Q are
// accessed -- see LiaBoard::enablePeripherals() and extra_variants/lia_v1/variant.cpp.
#define LIA_PIN_PPC 21

// Beacon Mode Selector switch (see firmware/AGENTS.md Phase 6 for behaviour).
#define LIA_PIN_BMS 15

// TP4056 charger status, open-drain active-low.
#define LIA_PIN_CHG 1
#define LIA_PIN_STBY 2

// Status LED, RED channel only. Common-anode: LOW at the pin lights it.
#define LIA_PIN_LED_RED 40

// -----------------------------------------------------------------------------
// Board identity
// -----------------------------------------------------------------------------
// Native USB (ESP32-S3 USB-Serial-JTAG) is used for the serial console --
// GPIO19/20 have no external USB-UART bridge on this board. If serial does
// not enumerate on first bring-up, try ARDUINO_USB_MODE=0 in platformio.ini
// (full USB-OTG/TinyUSB) instead -- see firmware/meshtastic/variants/esp32s3/lia_v1/platformio.ini.
