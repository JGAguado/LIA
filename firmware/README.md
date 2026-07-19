# LIA Firmware

Meshtastic-based firmware for the LIA pet tracker (ESP32-S3-MINI-1-N8). See
[`AGENTS.md`](AGENTS.md) for the full hardware spec, architecture, and
phase-by-phase roadmap, and [`docs/phase-status.md`](docs/phase-status.md)
for where the project currently stands against that roadmap.

## Repository layout

```
firmware/
├── AGENTS.md            Hardware spec, architecture, and roadmap (read this first)
├── board/                LiaBoard hardware abstraction (PPC, RED LED, BMS switch, charger status)
├── drivers/              Bespoke drivers (currently empty -- see drivers/README.md)
├── services/             TrackerService (periodic mesh transmissions)
├── docs/                 Phase validation notes
├── tools/                Build helper script
└── meshtastic/            Overlay applied on top of a Meshtastic firmware checkout
    ├── boards/lia_v1.json
    ├── variants/esp32s3/lia_v1/    (variant.h, pins_arduino.h, platformio.ini)
    └── extra_variants/lia_v1/     (variant.cpp -- earlyInitVariant() hook)
```

Meshtastic's own firmware (`meshtastic/firmware` on GitHub) is **not**
vendored into this repository -- it's a large, independently-versioned
project. Building for LIA means applying the files under `meshtastic/` and
`board/` on top of a checkout of it, the same way any out-of-tree Meshtastic
board variant is built.

## Building

`tools/build.ps1` automates the steps below (clone-if-missing, checkout the
pinned tag, copy the overlay, `pio run`). From a **native PowerShell**
session (not Git Bash/MSYS -- the ESP-IDF toolchain this platform pulls in
refuses to run under MSYS):

```powershell
cd firmware/tools
./build.ps1
```

Or manually:

1. Clone Meshtastic's firmware repository and check out a tagged release
   (`main` is a moving development branch):

   ```powershell
   git -c core.longpaths=true clone --recurse-submodules --depth 1 `
     --branch v2.7.26.54e0d8d https://github.com/meshtastic/firmware.git meshtastic-firmware
   ```

   `core.longpaths=true` is required on Windows -- some paths inside this
   repo exceed 260 characters.

2. Copy this repo's overlay into the checkout:

   ```powershell
   Copy-Item firmware/meshtastic/boards/lia_v1.json meshtastic-firmware/boards/
   New-Item -ItemType Directory -Force meshtastic-firmware/variants/esp32s3/lia_v1
   Copy-Item firmware/meshtastic/variants/esp32s3/lia_v1/* meshtastic-firmware/variants/esp32s3/lia_v1/
   New-Item -ItemType Directory -Force meshtastic-firmware/src/platform/extra_variants/lia_v1
   Copy-Item firmware/meshtastic/extra_variants/lia_v1/* meshtastic-firmware/src/platform/extra_variants/lia_v1/
   New-Item -ItemType Directory -Force meshtastic-firmware/src/lia
   Copy-Item firmware/board/* meshtastic-firmware/src/lia/
   New-Item -ItemType Directory -Force meshtastic-firmware/src/lia/services
   Copy-Item firmware/services/*.h, firmware/services/*.cpp meshtastic-firmware/src/lia/services/
   ```

3. Build:

   ```powershell
   cd meshtastic-firmware
   pio run -e lia_v1
   ```

4. Flash:

   ```powershell
   pio run -e lia_v1 -t upload --upload-port <PORT>
   ```

## Known assumptions to confirm on real hardware

These are documented where they're used, collected here for visibility:

- **USB mode** (`ARDUINO_USB_MODE=1`, USB-Serial-JTAG) -- confirmed working on
  real hardware (flashing and serial console both verified over it, Phase 0).
- **SX1262 DIO2/DIO3** (`SX126X_DIO2_AS_RF_SWITCH`,
  `SX126X_DIO3_TCXO_VOLTAGE 1.8`) -- inherited from the Wio-SX1262 module's
  reference design, not independently confirmed against LIA's schematic. See
  `meshtastic/variants/esp32s3/lia_v1/variant.h` and Phase 2.
- **BMS switch polarity** -- `firmware/AGENTS.md` and `MOD.md` disagree on
  which switch position is "Tracker" vs "Beacon" mode. See
  `board/README.md#open-questions` and Phase 6.
- **Charger status polarity** -- assumed open-drain active-low per the
  TP4056 datasheet. See `board/README.md#open-questions` and Phase 7.
