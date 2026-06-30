
[![License][license-shield]](LICENSE)
[![Project Maintenance][maintenance-shield]][maintenance]

# LIA

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/JGAguado/LIA/main/Style/Primary%20dark.png">
  <source media="(prefers-color-scheme: light)" srcset="https://raw.githubusercontent.com/JGAguado/LIA/main/Style/Primary%20light.png">
</picture>

**LIA** is an open-source low-power pet and asset tracker built on top of Meshtastic and LoRa mesh networking.

The project focuses on creating a compact, long-range, subscription-free tracking device capable of operating in off-grid environments using decentralized mesh communication.

---

## Features

- Long-range LoRa communication
- Meshtastic-powered mesh networking
- GPS location tracking
- Deep sleep low-power operation
- Subscription-free architecture
- Open-source hardware and firmware
- Compact wearable design
- Designed for outdoor tracking of pets.

---

## Hardware Stack

### Core Components

- ESP32-S3-MINI-1
- SX1262 LoRa transceiver
- u-blox SAM-M10Q GNSS module
- 18650 Li-ion battery
- USB-C / Magnetic port charging interface

### Antennas

- Internal FPC LoRa antenna
- Ceramic GNSS patch antenna

### Optional Sensors

- Accelerometer for motion-triggered updates
- Environmental sensors

---

## Software Stack

- Meshtastic firmware
- ESP-IDF
- PlatformIO
- Custom low-power tracker configurations

---

## Goals

LIA aims to provide:

- Offline-first tracking
- Community-powered coverage
- Lightweight wearable hardware
- Open ecosystem experimentation

---

## How It Works

1. Device wakes from deep sleep
2. GPS position is acquired
3. Location is broadcast through Meshtastic
4. Nearby nodes relay telemetry
5. Device returns to sleep

---
## License

MIT License

---

## Disclaimer

LIA is an experimental open-source project and should not be considered a guaranteed real-time safety tracking solution.


[license-shield]: https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg?style=for-the-badge

[maintenance-shield]: https://img.shields.io/badge/maintainer-J.%20G.%20Aguado-2e48a7.svg?style=for-the-badge
[maintenance]: https://github.com/JGAguado

