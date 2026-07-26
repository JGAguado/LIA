# LIA Firmware

Meshtastic-based firmware for the LIA pet tracker (ESP32-S3-MINI-1-N8). See
[`AGENTS.md`](AGENTS.md) for the full hardware spec, architecture, and
phase-by-phase roadmap, [`docs/phase-status.md`](docs/phase-status.md) for
detailed, evidence-quoted validation notes against that roadmap, and the
[Development History](https://jgaguado.github.io/LIA/docs/development-history/)
page on the docs site for the same story told chronologically.

## Two board variants

- **`lia_v1`** — the original board. Requires a solder rework crossing
  `SDA`/`SCL` back on the LSM6DSOXTR IMU's connection (miswired as
  manufactured, which caused I2C bus errors) before I2C -- and therefore the
  IMU and the MAX17048 battery gauge -- can be enabled. Do not assume I2C
  works on an unreworked board.
- **`lia_v2`** — the IMU physically removed (the same wiring defect's root
  cause), freeing the I2C bus for the battery gauge alone, no rework needed.

Both share all the same LIA-specific code below; only the pin definitions
differ (see `meshtastic/variants/esp32s3/*/variant.h`).

## Repository layout

```
firmware/
├── AGENTS.md                        Hardware spec, architecture, and roadmap (read this first)
├── board/                           LiaBoard: peripheral power, RED LED, BMS switch, charger status
├── drivers/                         ImuMotionDriver -- talks to the LSM6DSOXTR directly (see below)
├── services/
│   ├── MeshTargets.h                Shared target NodeNum + private channel name
│   ├── ChannelLookup.h/.cpp         Resolves a channel name to its index at runtime
│   ├── TrackerService.h/.cpp        Position broadcasts + the BMS state machine + manual LED override
│   ├── ChargeStatusService.h/.cpp   "Charging"/"Device charged" notifications
│   └── CommandService.h/.cpp        Text-command interface (lia_v1 only) -- GPS, BATTERY, IMU/CHG/STB/LED ON/OFF, HELP
├── docs/                            Phase validation notes
├── tools/
│   ├── build.ps1                    Build/flash helper (-Variant lia_v1|lia_v2, -Upload -Port)
│   ├── gps_test/                    Standalone GPS bring-up test (no Meshtastic)
│   └── battery_test/                Standalone MAX17048 bring-up test (no Meshtastic)
└── meshtastic/                       Overlay applied on top of a Meshtastic firmware checkout
    ├── boards/{lia_v1,lia_v2}.json
    ├── variants/esp32s3/{lia_v1,lia_v2}/    (variant.h, pins_arduino.h, platformio.ini)
    └── extra_variants/{lia_v1,lia_v2}/     (variant.cpp -- init hooks + deep-sleep wake sources)
```

Meshtastic's own firmware (`meshtastic/firmware` on GitHub) is **not**
vendored into this repository — it's a large, independently-versioned
project. Building for LIA means applying the files under `meshtastic/`,
`board/`, `drivers/`, and `services/` on top of a checkout of it, the same
way any out-of-tree Meshtastic board variant is built.

## Building

`tools/build.ps1` automates the steps below (clone-if-missing, checkout the
pinned tag, copy the overlay, `pio run`). From a **native PowerShell**
session (not Git Bash/MSYS -- the ESP-IDF toolchain this platform pulls in
refuses to run under MSYS):

```powershell
cd firmware/tools
./build.ps1                              # builds lia_v1 by default
./build.ps1 -Variant lia_v2               # or the other board
./build.ps1 -Upload -Port COM7            # build and flash in one step
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

2. Copy this repo's overlay into the checkout (`lia_v1` shown; substitute
   `lia_v2` for the other board):

   ```powershell
   Copy-Item firmware/meshtastic/boards/lia_v1.json meshtastic-firmware/boards/
   New-Item -ItemType Directory -Force meshtastic-firmware/variants/esp32s3/lia_v1
   Copy-Item firmware/meshtastic/variants/esp32s3/lia_v1/* meshtastic-firmware/variants/esp32s3/lia_v1/
   New-Item -ItemType Directory -Force meshtastic-firmware/src/platform/extra_variants/lia_v1
   Copy-Item firmware/meshtastic/extra_variants/lia_v1/* meshtastic-firmware/src/platform/extra_variants/lia_v1/
   New-Item -ItemType Directory -Force meshtastic-firmware/src/lia
   Copy-Item firmware/board/*.h, firmware/board/*.cpp meshtastic-firmware/src/lia/
   Copy-Item firmware/drivers/*.h, firmware/drivers/*.cpp meshtastic-firmware/src/lia/
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

## Provisioning

Set `device.role = TRACKER` and add a private channel named exactly `Test`
-- everything LIA-specific (position broadcasts, charge-status messages,
and every `CommandService` command) is gated on that channel, and fails
closed (skips the send, ignores incoming messages) if it isn't configured,
rather than silently falling back to the public default channel. See
[Meshtastic Configuration](https://jgaguado.github.io/LIA/docs/firmware/meshtastic-configuration/)
for why.

## Standalone bring-up tests

Two small, Meshtastic-free PlatformIO projects under `tools/` isolate
specific hardware questions from the rest of the firmware's complexity:

- **`tools/gps_test/`** -- powers the GNSS module and prints whatever it's
  decoded every 10 seconds.
- **`tools/battery_test/`** -- prints the MAX17048's charge percentage and
  cell voltage every 2 seconds via two independently-computed read paths
  side by side. This is what isolated a real gauge-reading bug (see
  [Battery](https://jgaguado.github.io/LIA/docs/firmware/battery/)).

Each has its own `platformio.ini` and README.

## Resolved hardware assumptions

These were open questions at some point during development and are noted
here for anyone reading history/commits rather than the current code:

- **BMS switch polarity** -- resolved in favor of `AGENTS.md`'s convention
  (HIGH = continuous, LOW = sleep-cycle) over an earlier, differently-worded
  planning doc.
- **Charger status polarity** (`CHG`/`STBY`) -- resolved against the actual
  TP4056 wiring: `CHG` active-low while charging, `STBY` pulled high
  externally while charging and pulled low by an internal switch on
  completion.
- **RED LED polarity** -- resolved against real hardware: the channel
  lights when driven high, not low as originally assumed.
- **I2C bus errors on `lia_v1`** -- resolved via a solder rework crossing
  `SDA`/`SCL` back on the IMU's connection; see "Two board variants" above.

Still open: **SX1262 DIO2/DIO3** (`SX126X_DIO2_AS_RF_SWITCH`,
`SX126X_DIO3_TCXO_VOLTAGE 1.8`) are inherited from the Wio-SX1262 module's
reference design, not independently confirmed against LIA's own schematic.
