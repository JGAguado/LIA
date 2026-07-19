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
| 2 -- SX1262 Integration | Done, verified end-to-end on real hardware (see below) | Done -- user paired via the official app, set the region, and confirmed receiving "LIA Radio Test" on the destination node |
| 3 -- Radio Validation | N/A (physical range/sleep-wake test only) | Stable RF communication confirmed (real TX + real RX with rxSNR=6/rxRSSI=-49 from a 39-node mesh, see Phase 5 notes); range/sleep-wake walk test still pending |
| 3.5 -- TrackerService | Done, confirmed sending every 30s on real hardware (see below) | N/A |
| 4 -- SAM-M10Q Integration | Done -- nothing to build, stock Meshtastic auto-detects the module (`GNSS_MODEL_UBLOX10`) and handles NMEA/UBX via the Phase 0 pin config | Cold/warm fix timing and PPC power-cycling still need outdoor testing |
| 5 -- GPS + Meshtastic | Done, `TrackerService` now sends real `meshtastic_Position` packets (see below) | GPS icon / actual position receipt still pending an outdoor fix |
| 6 -- Tracker Behaviour | Done, BMS-HIGH continuous path confirmed on real hardware; BMS-LOW sleep-cycle path implemented + code-verified but not yet observed running (needs the physical switch flipped, see below) | Toggling the switch and confirming the LOW-side sleep/wake cycle still pending |
| 7 -- Charging Behaviour | Not started | Pending |
| 8 -- Deep Sleep | Partially done as a side effect of Phase 6 (RTC-timer wake via `doDeepSleep`); BMS/USB wake sources still pending | Pending |
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
- **User confirmed real end-to-end delivery**: paired with the official
  Meshtastic app, set the region, and received "LIA Radio Test" on the
  destination node (MAC `E7:25:DC:E6:D6:63`). This is genuine, independent
  confirmation from the receiving side of the link, not just our own serial
  log.

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

## Phase 4 notes

No new code. `src/gps/GPS.cpp` already auto-probes and identifies the
SAM-M10Q as `GNSS_MODEL_UBLOX10`, then runs stock NMEA/UBX handling -- all
switched on purely by the `GPS_RX_PIN`/`GPS_TX_PIN`/`HAS_GPS` defines already
in `variant.h` since Phase 0. The "powersave" NAK observed in earlier
captures is a benign quirk in that stock code path (not ours to patch, per
"never edit Meshtastic core").

## Phase 5 notes

- `TrackerService`'s constructor now uses `meshtastic_PortNum_POSITION_APP`
  instead of `TEXT_MESSAGE_APP`. `runOnce()` builds a `meshtastic_Position`
  from the global `localPosition` (`NodeDB.h`, kept current by the GPS
  subsystem) and `pb_encode_to_bytes`s it into the packet payload, matching
  stock `PositionModule.cpp`'s own pattern exactly. Skips the send (logging
  `TrackerService: no GPS fix yet, skipping send`) when latitude/longitude
  are both still zero -- the same guard stock Meshtastic uses, since a (0,0)
  position is meaningless.
- **Confirmed working on real hardware**, and this capture was the first to
  show real RF activity end to end, not just gated attempts:
  ```
  [RadioIf] Started Tx (...)
  [RadioIf] Packet TX: 1009ms
  [RadioIf] Completed sending (...)
  [RadioIf] Lora RX (... rxSNR=6 rxRSSI=-49 ...)
  [Router] Rx someone rebroadcasting for us (...)
  [DeviceTelemetry] Node status update: 39 online, 39 total
  [Tracker] TrackerService: no GPS fix yet, skipping send
  ```
  The region is clearly set now (no more "Region unset" warnings anywhere in
  the capture) and the board is hearing a real 39-node mesh with a strong
  received signal -- this is a substantial, real confirmation of Phase 3
  ("stable RF communication"), not just Phase 5. `TrackerService` correctly
  skipped sending a position (no GPS fix indoors) instead of sending a
  meaningless (0,0) -- exactly the intended behavior; not yet verified with
  an actual fix, which needs open sky.

## Phase 6 notes

- Resolved the BMS polarity question 2026-07-19 per explicit instruction:
  follow `firmware/AGENTS.md` as authoritative, disregard `MOD.md`'s opposite
  naming (see `board/README.md#open-questions`).
- Researched rather than guessed how a `TRACKER`-role Meshtastic device is
  meant to sleep, since inventing bespoke sleep logic risked fighting stock
  `PowerFSM`: `PowerFSM.cpp` explicitly skips adding its own light-sleep
  timers for `TRACKER`/`TAK_TRACKER`/`SENSOR` roles ("sleep will be initiated
  through the modules"), and `PositionModule.cpp` shows the exact expected
  pattern for a tracker module to follow -- send, then `setIntervalFromNow`
  a short grace period, then call `doDeepSleep()` on the *next* tick (so the
  packet has time to actually get on air first). `TrackerService`'s BMS-LOW
  path mirrors this pattern.
- `LiaBoard` now registers a `notifyDeepSleep` observer in `begin()` that
  calls `disablePeripherals()` (cuts PPC, the shared SX1262+GPS rail) before
  any deep sleep actually happens. Confirmed this fires for us specifically:
  `sleep.cpp` only *skips* calling `notifyDeepSleep` when `shouldLoraWake()`
  is true, which by its own source is only true for `ROUTER`/`ROUTER_LATE`
  roles -- not `TRACKER`.
- `TrackerService::runOnce()` now branches on `LiaBoard::instance().isBmsHigh()`:
  - **HIGH ("continuous")**: RED LED solid on, keep the existing 30s send
    cadence indefinitely, never sleep.
  - **LOW ("sleep-cycle")**: send once (or wait up to 90s for a first fix,
    `kFixWaitTimeoutMs`), then deep sleep 60s (`kSleepCycleMs`) and let the
    next wake's fresh boot repeat the cycle.
- Build succeeded clean on the first try (no compile errors, unlike Phase 2's
  two-bug episode) -- another `Could not open COM4` USB hiccup blocked the
  reflash temporarily, cleared by a replug as usual.
- **Confirmed on real hardware, BMS-HIGH path only**: the current physical
  switch position is HIGH. Serial log shows `TrackerService: no GPS fix yet`
  at uptime 31s, 61s, and 91s -- each exactly 30s apart, matching
  `kContinuousIntervalMs`. This is the evidence it's the continuous path
  specifically (the BMS-LOW branch would retry every 5s instead while still
  within the fix-wait window). No crash, no attempted sleep -- correct per
  spec. RED LED should be solid on now; not visually confirmed (no camera on
  this end).
- **BMS-LOW sleep-cycle path is implemented and compiles, but not yet
  observed actually running** -- that needs the physical switch flipped to
  LOW, which only the user can do. Expect the board to go briefly silent on
  serial (deep sleep fully powers down the USB-Serial-JTAG peripheral, so the
  USB connection will legitimately drop, not just go quiet) and come back
  with a fresh boot sequence roughly a minute later.
- Also requires `device.role` set to `TRACKER` via the app/CLI for the sleep
  behavior to work as intended (see `services/README.md`) -- not yet
  confirmed this has been set; if it's still the default role, stock
  `PowerFSM` may add its own competing light-sleep transitions.
