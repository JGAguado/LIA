
# AGENTS.md
You are an firmware expert specialized on embedded systems and microcontrollers. This document provides a comprehensive guide for developing firmware for the LIA (Off-grid. On Track.) pet tracker, which is built around the ESP32-S3-MINI-1-N8 microcontroller. The firmware is based on the Meshtastic platform and aims to maintain compatibility with upstream Meshtastic while implementing board-specific features.

# LIA Firmware Development Guide

## Project
LIA (Off-grid. On Track.) is an open-source Meshtastic-based pet tracker built around a custom ESP32-S3 board.

## Goals
- Stay as close as possible to upstream Meshtastic.
- Implement board-specific features through isolated modules.
- Deliver a working firmware after every phase.
- Prioritize low power, maintainability and documentation.

## Hardware Constraints

Current PCB revision limitations:

- Ignore the entire I²C bus.
- Ignore GREEN LED.
- Ignore BLUE LED.

These peripherals SHALL NOT be initialized nor referenced.

## Functional Hardware

> **Module pin ≠ GPIO number.** The table below is silicon GPIO numbers, not
> ESP32-S3-MINI-1-N8 module pin numbers — the two do not line up 1:1 (e.g.
> module pin 12 carries `RESET`, but that pin is silicon GPIO8, not GPIO12).
> Verified against the `ESP32-S3-MINI-1-N8` symbol pin definitions and net
> labels in `hardware/kicad/LIA.kicad_sch` (component `U1`).

| GPIO | Function |
|------|----------|
| GPIO8 | SX1262 RESET |
| GPIO9 | SX1262 BUSY |
| GPIO10 | SX1262 NSS |
| GPIO11 | SX1262 MOSI |
| GPIO12 | SX1262 SCK |
| GPIO13 | SX1262 MISO |
| GPIO14 | SX1262 DIO1 |
| GPIO17 | GPS TX |
| GPIO18 | GPS RX |
| GPIO21 | PPC (Peripheral Power Control) |
| GPIO40 | RED LED |
| GPIO1 | CHG |
| GPIO2 | STBY |
| GPIO15 | BMS Switch |

PPC MUST be HIGH before using either the SX1262 or the SAM‑M10Q.

## Firmware Architecture

Meshtastic Core
→ Board Definition
→ LiaBoard (hardware abstraction)
→ Drivers
→ TrackerService

LiaBoard owns:
- enablePeripherals()
- disablePeripherals()
- setRedLed()
- isCharging()
- isChargeComplete()
- isTrackerMode()

TrackerService owns:
- destination selection
- tracker state machine
- GPS transmission scheduling

## Development Rules

- Never edit Meshtastic core unless unavoidable.
- Prefer board definitions, modules and services.
- No GPIO access outside drivers/LiaBoard.
- Modern C++17.
- No blocking delay().
- Document every module.
- Every phase must end with a working firmware.

## Flashing & Validation Workflow

- The agent owns build + flash + serial-log verification at the end of every
  phase: `pio run -e lia_v1 -t upload --upload-port <PORT>`, then capture
  serial output (115200 baud) across a reset to confirm boot/init and check
  for crash loops before calling a phase's firmware deliverable done.
- The agent does not wait for the user to flash manually -- it flashes the
  connected board itself and reports what the serial log showed.
- What still requires the user, because it needs physical presence or a
  second device: phone-app pairing, reading LED colour/pattern, RF range and
  walk testing, current measurement, toggling the physical BMS switch,
  anything needing the configured destination node to confirm receipt.
- Each phase's "User Validation" checklist splits along that line: treat
  build/flash/serial-boot confirmation as covered by the agent, the rest as
  still open until the user confirms it.

## Target Node

Development target:

MAC:
E7:25:DC:E6:D6:63

Create a single configuration constant for the destination.
Future versions should migrate to configurable Meshtastic Node ID.

---

# Development Roadmap

## Phase 0 – Board Bring-up

Deliverables:
- PlatformIO project
- USB serial
- Flashing
- Boot
- Reset

Validation:
- Board flashes successfully.
- Serial console operational.

---

## Phase 1 – PPC Validation

Deliverables:
- GPIO25 controls peripheral power.
- Verify SX1262 and GPS lose power when PPC LOW.

Validation:
- PPC HIGH powers peripherals.
- PPC LOW removes power.

---

## Phase 2 – SX1262 Integration

Deliverables:
- Board behaves as a normal Meshtastic node.
- SPI validated.
- Radio TX/RX operational.
- Every 30 seconds send "LIA Radio Test" as a unicast to the configured target node.

User Validation:
- Pair with official Meshtastic app.
- Rename node.
- Save configuration.
- Broadcast message TX/RX.
- Destination node receives "LIA Radio Test".
- RSSI/SNR visible.

---

## Phase 3 – Radio Validation

Deliverables:
- Stable RF communication.
- Range verification.
- Sleep/wake validation.

User Validation:
- Walk test.
- Packet loss acceptable.
- No radio resets.

---

## Phase 3.5 – TrackerService

Deliverables:
- Create TrackerService.
- Encapsulate periodic transmissions.
- Payload currently "LIA Radio Test".
- Destination configurable from one location.

---

## Phase 4 – SAM-M10Q Integration

Deliverables:
- UART configured.
- NMEA parsing.
- UBX communication.
- Cold/Warm starts validated.

User Validation:
- GPS fix acquired.
- Power cycling works.
- PPC dependency verified.

---

## Phase 5 – GPS + Meshtastic

Deliverables:
- GPS registered with Meshtastic.
- Official app config works.
- TrackerService replaces test payload with GPS position.

User Validation:
- GPS icon visible.
- Position updates.
- Position interval configurable.
- Destination node receives GPS positions.

---

## Phase 6 – Tracker Behaviour

BMS HIGH:
- RED LED ON.
- GPS continuously active.
- Continuous position streaming.
- No deep sleep.

BMS LOW:
- Wake every minute.
- PPC HIGH.
- GPS fix.
- Send position.
- SX1262 sleep.
- GPS off.
- ESP32 deep sleep.

User Validation:
- Toggle switch without reflashing.
- Behaviour changes immediately.

---

## Phase 7 – Charging Behaviour

Superseded 2026-07-22 by explicit instruction: no LED indication for this
phase. Instead, report status as a mesh text message to the configured
target node (see "Target Node"), on the same private channel TrackerService
uses (`ChannelLookup`/`MeshTargets.h`):
- Send `"Charging"` when CHG is detected (edge-triggered, once per
  charging-session start).
- Send `"Device charged"` when STBY reads HIGH (edge-triggered, once per
  completion).

User Validation:
- Verify "Charging" message arrives when a charger is connected.
- Verify "Device charged" message arrives once charging completes.

---

## Phase 8 – Deep Sleep

Deliverables:
- RTC wake.
- BMS wake.
- USB wake.
- Radio sleep.
- GPS power-down.

User Validation:
- Measure current.
- Verify wake sources.

---

## Phase 9 – Power Validation

Measure:
- Tracker current.
- Sleep current.
- GPS acquisition time.
- Estimated battery life.

---

## Phase 10 – Field Trial

Real-world dog tracking validation.

Checklist:
- Continuous mode follows movement.
- Low-power mode reports every minute.
- Target node receives all positions.
- No firmware crashes.

## Coding Standards

- ESP-IDF / PlatformIO.
- constexpr where possible.
- enum class.
- RAII.
- clang-format.
- Doxygen.
- No globals.

## Repository Layout

firmware/
- board/
- drivers/
- services/
- meshtastic/
- docs/

Each driver contains:
- README.md
- examples
- tests

## Definition of Done

Every phase requires:
- Deliverables complete.
- User validation PASS.
- No regressions.
- Documentation updated.

## Final Success Criteria

The firmware shall:

- Operate as a standard Meshtastic node.
- Be configurable through the official Meshtastic application.
- Continuously stream GPS while BMS HIGH.
- Stream once per minute and sleep while BMS LOW.
- Show RED LED solid during tracker mode.
- Show RED LED breathing while charging.
- Turn RED LED OFF when fully charged.
- Keep minimal divergence from upstream Meshtastic.
