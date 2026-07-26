# LIA Firmware

Meshtastic-based firmware for the LIA pet tracker (ESP32-S3-MINI-1-N8). See
[`AGENTS.md`](AGENTS.md) for the full hardware spec, architecture, and
phase-by-phase roadmap, [`docs/phase-status.md`](docs/phase-status.md) for
detailed, evidence-quoted validation notes against that roadmap, and the
[Development History](https://jgaguado.github.io/LIA/docs/development-history/)
page on the docs site for the same story told chronologically.

## One board variant, two hardware revisions

There is a single firmware/board variant, `lia_v1`, and it's the correct
build for every LIA board. It requires a solder rework crossing `SDA`/`SCL`
back on the LSM6DSOXTR IMU's connection (miswired as manufactured, which
caused I2C bus errors) before I2C -- and therefore the IMU and the MAX17048
battery gauge -- can be enabled; both hardware revisions in circulation now
have that rework done. Do not assume I2C works on an unreworked board.

The two hardware revisions, **V1.0** and **V1.1**, are a **battery/enclosure
distinction only** (see
[Battery](https://jgaguado.github.io/LIA/docs/firmware/battery/) and
[Enclosure](https://jgaguado.github.io/LIA/docs/hardware/enclosure/)) -- V1.0
takes an 18650 3500mAh cell, V1.1 a 5023450 1000mAh cell. Nothing about the
firmware differs between them; there is no `lia_v2` variant.

## Repository layout

```
firmware/
├── AGENTS.md                        Hardware spec, architecture, and roadmap (read this first)
├── board/                           LiaBoard: peripheral power, RED LED, BMS switch, charger status
├── drivers/                         ImuMotionDriver -- talks to the LSM6DSOXTR directly (see below)
├── services/
│   ├── MeshTargets.h                Shared target NodeNum (kLiaTargetNode) -- see "Provisioning" below
│   ├── TrackerService.h/.cpp        Position broadcasts + the BMS state machine + manual LED override
│   ├── ChargeStatusService.h/.cpp   "Charging"/"Device charged" notifications
│   └── CommandService.h/.cpp        Text-command interface -- GPS, BATTERY, IMU/CHG/STB/LED ON/OFF, HELP
├── docs/                            Phase validation notes
├── tools/
│   ├── build.ps1                    Build/flash helper (-Upload -Port)
│   ├── gps_test/                    Standalone GPS bring-up test (no Meshtastic)
│   └── battery_test/                Standalone MAX17048 bring-up test (no Meshtastic)
└── meshtastic/                       Overlay applied on top of a Meshtastic firmware checkout
    ├── boards/lia_v1.json
    ├── variants/esp32s3/lia_v1/    (variant.h, pins_arduino.h, platformio.ini)
    └── extra_variants/lia_v1/     (variant.cpp -- init hooks + deep-sleep wake sources)
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
./build.ps1                              # builds lia_v1 -- correct for both V1.0 and V1.1 boards
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

2. Copy this repo's overlay into the checkout:

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

Set `device.role = TRACKER`. Everything LIA-specific (position broadcasts,
charge-status messages, and every `CommandService` command/reply) is a
direct message to a single hardcoded target node (`MeshTargets.h`'s
`kLiaTargetNode`, derived from that node's MAC), not a channel broadcast --
building your own copy means editing that constant to your own target
node's NodeNum first. See
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
  `SDA`/`SCL` back on the IMU's connection; see "One board variant, two
  hardware revisions" above.

Still open: **SX1262 DIO2/DIO3** (`SX126X_DIO2_AS_RF_SWITCH`,
`SX126X_DIO3_TCXO_VOLTAGE 1.8`) are inherited from the Wio-SX1262 module's
reference design, not independently confirmed against LIA's own schematic.
