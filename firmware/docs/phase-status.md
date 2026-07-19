# Phase status

Tracks each roadmap phase in `../AGENTS.md` against its Definition of Done
(deliverables complete, user validation pass, no regressions, docs updated).
"User validation" always requires physical hardware and is out of reach for
an AI agent -- those boxes can only be checked by whoever has the board in
hand.

| Phase | Code deliverables | User validation |
| --- | --- | --- |
| 0 -- Board Bring-up | Done, flashed and confirmed booting on real hardware (COM4) | Done (agent-verified via serial log; see below) |
| 1 -- PPC Validation | Done -- indirectly confirmed: GPS and radio both respond over their buses, which requires PPC/power to be up (see below) | Multimeter check still pending, but functionally demonstrated |
| 2 -- SX1262 Integration | Done, verified end-to-end on real hardware (see below) | Region/app-pairing still pending (agent can set region via CLI if asked -- regulatory, so not done unprompted) |
| 3 -- Radio Validation | N/A (physical range/sleep-wake test only) | Pending |
| 3.5 -- TrackerService | Done, confirmed sending every 30s on real hardware (see below) | N/A |
| 4 -- SAM-M10Q Integration | Not started | Pending |
| 5 -- GPS + Meshtastic | Not started | Pending |
| 6 -- Tracker Behaviour | Not started (blocked on BMS polarity -- see `board/README.md#open-questions`) | Pending |
| 7 -- Charging Behaviour | Not started | Pending |
| 8 -- Deep Sleep | Not started | Pending |
| 9 -- Power Validation | N/A (physical measurement only) | Pending |
| 10 -- Field Trial | N/A (physical trial only) | Pending |

## Phase 0 notes

- PlatformIO `lia_v1` environment created under `../meshtastic/`.
- `LiaBoard` singleton created under `../board/`, wired via
  `earlyInitVariant()` so PPC is high before Meshtastic's own SPI/radio init
  runs.
- Build verified clean against a fresh `meshtastic/firmware @
  v2.7.26.54e0d8d` checkout: `pio run -e lia_v1` succeeded (4m36s), producing
  a flashable `firmware-lia_v1-2.7.26.54e0d8d.factory.bin`. RAM 40.3% (132024
  / 327680 B), Flash 62.8% (2098013 / 3342336 B). One fix was needed along
  the way: `pins_arduino.h` had to declare `SDA`/`SCL` pin constants even
  though I2C is unused, because Arduino-ESP32's `Wire.cpp` references them
  unconditionally at compile time -- this does not initialize or scan the
  bus (see the comment in `pins_arduino.h`).
- **Flashed to real hardware and confirmed booting**, via the agent (see
  `AGENTS.md` "Flashing & Validation Workflow"): a board was connected on
  COM4 (ESP32-S3, 8MB flash, USB-Serial/JTAG, MAC 38:44:be:c8:16:84 -- matches
  our board config exactly). `pio run -e lia_v1 -t upload --upload-port COM4`
  succeeded, and a serial capture showed a clean boot: NodeInfo broadcast,
  packet history, AES128 key expansion, and the radio subsystem correctly
  gating TX with `lora tx disabled: Region unset` (expected -- no region
  configured yet). No crash, no reboot loop.

## Phase 2 notes

- Added `LiaTestModule` (`SinglePortModule` + `OSThread`, in
  `extra_variants/lia_v1/variant.cpp`) sending "LIA Radio Test" as a unicast
  to NodeNum `0xDCE6D663` (the low 32 bits of target MAC `E7:25:DC:E6:D6:63`,
  matching `NodeDB.cpp`'s own derivation) every 30s, instantiated from
  `lateInitVariant()`.
- **First on-device test crashed** (`Guru Meditation Error: LoadProhibited`,
  reboot loop). Diagnosed with `xtensa-esp32s3-elf-addr2line` against the
  built `.elf` rather than guessing from logs alone:
  1. First fix (necessary but not sufficient): `runOnce()` dereferenced the
     global `airTime` before it's constructed. `lateInitVariant()` runs
     right after `initLoRa()` in `main.cpp`, but `airTime = new AirTime()`
     doesn't happen until ~40 lines later -- `router` and `service` are
     already valid by then, `airTime` isn't. Added a null guard.
  2. Real root cause (found after the crash persisted, addr2line pointed
     straight at it): `earlyInitVariant()`'s `LOG_INFO(...)` call was crashing
     in `RedirectablePrint::log()`. `earlyInitVariant()` is the *second* call
     in `setup()` (right after `waitUntilPowerLevelSafe()`), well before the
     console/logging subsystem is up -- logging that early isn't safe on this
     platform. Removed the log call from `earlyInitVariant()`; moved the
     equivalent confirmation line into `lateInitVariant()`, which is known-
     good (Phase 0's boot log already showed plenty of successful logging
     from that point on).
- Both fixes are in and the build is clean. On-device confirmation was
  temporarily blocked by 4 consecutive esptool write-timeouts on COM4 (the
  Windows USB/serial stack got wedged by the rapid crash/reboot cycling, not
  a build or hardware fault) -- a physical USB replug (user's step) cleared
  it immediately, same port/serial number.
- **Confirmed working on real hardware** after the replug: clean boot, no
  crash, and the serial log shows the full pipeline succeeding:
  ```
  [LiaTest] Setting next hop for packet with dest dce6d663 to 0
  [LiaTest] send - lora tx disabled: Region unset
  [LiaTest] LiaTestModule: sent "LIA Radio Test" to 0xdce6d663
  ```
  Destination NodeNum matches the derivation exactly. Gated on "Region
  unset" is correct/expected Meshtastic behavior, not a bug -- actual RF
  transmission needs a region set first (regulatory, see `firmware`'s
  lora.md-equivalent guidance) which hasn't been done since that's a
  real-world RF-emission decision, not a build step.
- Bonus signal for Phase 1/4: the log also shows the GPS module actively
  responding over UART (NAK on a powersave command, but real two-way
  traffic, then "GNSS module configuration saved!") -- since GPS is on the
  PPC-gated rail alongside the SX1262, this is de facto evidence PPC control
  is working, ahead of an actual multimeter check.

## Phase 3.5 notes

- Moved the sender out of `extra_variants/lia_v1/variant.cpp` into
  `services/TrackerService.{h,cpp}` (same `SinglePortModule` + `OSThread`
  logic, renamed `LiaTestModule` -> `TrackerService`, module name "Tracker").
  `lateInitVariant()` now just does `new TrackerService();`. Build succeeded
  clean (including both new files) on the first try -- no new bugs, since
  this was a pure move/rename of already-verified code.
- On-device reflash hit a *different* USB hiccup first: `ClearCommError
  failed (PermissionError...)` on connect. `Get-PnpDevice` showed the board's
  USB composite device and JTAG/serial debug interface both `Status: OK`,
  but the `Ports` (COM4) sub-interface specifically `Status: Unknown` -- same
  class of Windows USB-serial wedge as the Phase 2 episode, not a build
  problem. Cleared by another physical replug.
- **Confirmed working on real hardware**: two sends 31s apart
  (`uptime 31.307s` then `62.211s`, matching the 30000ms return from
  `runOnce()`), both logged as `[Tracker] TrackerService: sent "LIA Radio
  Test" to 0xdce6d663`. No crash.
- New (unrelated to our code) warning observed in this capture:
  `SX126x AGC reset: calibration did not complete within 50ms`, from stock
  RadioLib. Not a crash and didn't block the send, but worth watching once a
  region is set and real RF is active -- possibly related to the Wio-SX1262
  TCXO timing assumption already flagged in `variant.h` for Phase 2.
