---
title: Microcontroller
description: ESP32-S3-MINI-1-N8 pin assignment on the LIA main board.
---

LIA is built around an **ESP32-S3-MINI-1-N8** (`U1`), which runs the Meshtastic firmware and talks to every other peripheral on the board over SPI, I2C, and UART. This page documents how its pins are wired, straight from the `uController` schematic sheet.

## Pinout

:::note
**Module pin ≠ GPIO number.** The "Module pin" column below is the
ESP32-S3-MINI-1-N8's own physical pin number (matching the schematic symbol),
which does **not** line up 1:1 with the GPIO number firmware actually uses —
e.g. module pin 8 carries the `SDA` net, but that pin is silicon **GPIO4**, not
GPIO8. The GPIO column was derived by cross-referencing this table against the
ESP32-S3-MINI-1-N8 symbol's own pin definitions embedded in
`hardware/kicad/LIA.kicad_sch`, and is what's actually used in
`firmware/meshtastic/variants/lia_v1/variant.h`.
:::

| Module pin | GPIO   | Signal  | Function                                          |
| ---------- | ------ | ------- | -------------------------------------------------- |
| 3          | —      | 3V3     | 3.3 V supply                                        |
| 45         | —      | EN      | Enable/reset — 10 kΩ pull-up, 1 µF decouple, reset button |
| 4          | GPIO0  | IO0     | Boot mode strap + BOOT button                       |
| 8          | GPIO4  | SDA     | I2C data — shared bus (IMU and fuel gauge, both boards — see [Battery](/LIA/docs/firmware/battery/)) |
| 9          | GPIO5  | SCL     | I2C clock — shared bus                              |
| 10         | GPIO6  | IMU_INT | Accelerometer interrupt (see [Accelerometer](/LIA/docs/firmware/accelerometer/)) |
| 12         | GPIO8  | RESET   | LoRa radio reset (SX1262)                           |
| 13         | GPIO9  | BUSY    | LoRa radio busy (SX1262)                            |
| 14         | GPIO10 | NSS     | LoRa radio SPI chip select (SX1262)                 |
| 15         | GPIO11 | MOSI    | LoRa radio SPI MOSI (SX1262)                        |
| 16         | GPIO12 | SCK     | LoRa radio SPI clock (SX1262)                       |
| 17         | GPIO13 | MISO    | LoRa radio SPI MISO (SX1262)                        |
| 18         | GPIO14 | DIO     | LoRa radio DIO interrupt (SX1262)                   |
| 21         | GPIO17 | Tx      | UART to GNSS module (SAM-M10Q)                      |
| 22         | GPIO18 | Rx      | UART from GNSS module (SAM-M10Q)                    |
| 23         | GPIO19 | IO19    | USB D−                                              |
| 24         | GPIO20 | IO20    | USB D+                                              |
| 5          | GPIO1  | CHG     | Battery charger status                              |
| 6          | GPIO2  | STBY    | Battery charger standby status                      |
| 19         | GPIO15 | BMS     | Beacon Mode Selector — physically 3-position; see [Power Management](/LIA/docs/firmware/power-management/) for the OFF position |
| 25         | GPIO21 | PPC     | Peripheral power control                            |
| 36         | GPIO40 | Lred    | Status LED, red channel — see [RGB LED](/LIA/docs/firmware/rgb-led/) |
| 37         | GPIO41 | Lgreen  | Status LED, green channel — see [RGB LED](/LIA/docs/firmware/rgb-led/) |
| 38         | GPIO42 | Lblue   | Status LED, blue channel — see [RGB LED](/LIA/docs/firmware/rgb-led/) |
| 39         | —      | TXD0    | UART0 (reserved, not connected in this design)      |
| 40         | —      | RXD0    | UART0 (reserved, not connected in this design)      |
| —          | —      | GND     | Ground (multiple pins)                              |


See also: [BOM](/LIA/docs/hardware/bom/), [Schematics](/LIA/docs/hardware/schematics/), [LoRa](/LIA/docs/firmware/lora/), [GPS](/LIA/docs/firmware/gps/), and [Building the Firmware](/LIA/docs/firmware/building-the-firmware/) (this pinout is what the `lia_v1` board variant is built from).
