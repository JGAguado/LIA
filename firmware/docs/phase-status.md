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
| 6 -- Tracker Behaviour | Done, both BMS-HIGH and BMS-LOW paths confirmed on real hardware across multiple full sleep/wake cycles (see below) | Toggle-without-reflashing confirmed for BMS LOW; HIGH<->LOW toggle-while-running not yet explicitly re-tested |
| 7 -- Charging Behaviour | Not started | Pending |
| 8 -- Deep Sleep | Partially done as a side effect of Phase 6 (RTC-timer wake via `doDeepSleep`, confirmed repeatable); BMS-wake and USB-wake sources still pending | Pending |
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
- **BMS-LOW sleep-cycle path: first attempt crashed.** With the switch
  flipped to LOW (during a later GPS-test session), the board got a real GPS
  fix (36s to lock, 12 satellites) and `TrackerService` sent a real,
  non-(0,0) position -- then crashed entering deep sleep:
  ```
  SX126x entering sleep mode
  SX126x standby RadioLib err=-707
  assert failed: void SX126xInterface<T>::setStandby() ... err == RADIOLIB_ERR_NONE
  ```
  Root cause (found by grepping for the log string rather than guessing):
  `RadioInterface::init()` (called during `initLoRa()`, before
  `lateInitVariant()`) and `GPS::setup()` **each register their own
  `notifyDeepSleep` observer**, to gracefully command the SX1262/GNSS to
  standby over SPI/UART while still powered. `Observable` fires observers in
  registration order. `LiaBoard`'s observer was registered in `begin()`
  (called from `earlyInitVariant()`, which runs *before* both of those) --
  so PPC was being cut *before* RadioInterface got a chance to send its
  graceful SPI standby command, and that command failed against unpowered
  hardware, tripping Meshtastic's own internal assert.
  **Fix**: split the registration out into a new
  `LiaBoard::armDeepSleepHook()`, called from `lateInitVariant()` instead --
  which runs after both `initLoRa()` and `GPS::createGps()`/`setup()`, so
  those two observers are already registered (and will fire) before ours.
- **Confirmed fixed and fully working across two complete sleep/wake
  cycles**, still on the BMS-LOW switch position:
  - Cycle 1: real fix (lat=47.842260, lon=16.259864, alt=287m) sent
    successfully, then `SX126x entering sleep mode` (no error this time),
    `GPS deep sleep!` / `GPS power state move from HARDSLEEP to OFF`, prefs
    saved cleanly, then the serial connection genuinely dropped (real deep
    sleep powers off the USB-Serial-JTAG peripheral -- confirmed via
    `Get-PnpDevice` showing all three USB interfaces go `Unknown`).
  - The board came back ~70-80s later (`Get-PnpDevice` interfaces back to
    `OK`) with no manual replug needed -- a genuine RTC-timer wake, not a
    crash-reboot.
  - Cycle 2: reconnected mid-cycle and caught it already running again
    (uptime ~50s, sending `DeviceTelemetry`), then cleanly re-entering the
    same sleep sequence a second time -- confirms the cycle repeats
    correctly, not just a one-off.
- Also requires `device.role` set to `TRACKER` via the app/CLI for the sleep
  behavior to work as intended (see `services/README.md`) -- not explicitly
  re-confirmed after this fix, but the observed behavior (clean sleep, no
  competing PowerFSM light-sleep transitions) is consistent with it already
  being set.

## Phase 6 addendum: channel-targeted position sending (2026-07-20)

User asked: position was reaching the destination node fine, but since
Meshtastic encrypts per-channel (not per-recipient) and `PositionModule` is
`isPromiscuous` (any node updates its map from any overheard position packet
regardless of the packet's `to` field), sending on the default/public
channel meant *any* node on the mesh could map this device, not just the
intended destination. Requested restricting visibility to other users on a
private channel already configured on the device, named `"Test"`, looked up
dynamically since it could be at any of the 8 channel slots.

- `TrackerService::findChannelIndexByName()` scans `channels.getNumChannels()`
  entries via `channels.getName(i)` (case-insensitive), returning the index of
  the first match or `-1`. `sendPosition()` fails closed on `-1` -- logs a
  warning and skips the send rather than falling back to the default
  channel, since that fallback would silently defeat the point.
- Kept the existing unicast `p->to = kDestination` unchanged; only added
  `p->channel = <found index>`, matching stock `PositionModule::sendOurPosition()`'s
  own `if (channel > 0) p->channel = channel;` pattern (confirmed fresh
  against `PositionModule.cpp:342-372` in the checkout before implementing).
- **Confirmed on real hardware**: the device's "Test" channel resolved to
  index 1. Serial log:
  ```
  [Tracker] TrackerService: sent position (lat=478422330, lon=162600680) to 0xdce6d663 on channel 1 ("Test")
  ```
  repeated twice, 30s apart. The actual TX packets show `Ch=0x7d` (the
  "Test" channel's PSK-derived hash), visibly different from the default
  channel's `Ch=0x8` seen on this same boot's NodeInfo broadcast --
  confirming the position packets are encrypted under a different key than
  the public default channel, not just logged as if they were. Real GPS fix
  acquired during this same capture (36s to lock, 10 satellites,
  lat=47.842233, lon=16.260068), and real RF RX/rebroadcast activity
  observed (`Rx someone rebroadcasting for us`).

## Phase 7 notes

Superseded per explicit instruction (2026-07-22): no RED LED breathing/off
behaviour, since `TrackerService` already owns the RED LED for BMS-mode
indication (Phase 6) and the user asked for status as messages instead. New
`ChargeStatusService` (`SinglePortModule` + `OSThread`, same pattern as
`TrackerService`) polls `LiaBoard::instance().isCharging()` /
`isChargeComplete()` every 3s and sends edge-triggered text -- `"Charging"`
once when charging starts, `"Device charged"` once when charge-complete is
detected -- to `kLiaTargetNode` on the private `kLiaChannelName` channel
(the same shared destination/channel as `TrackerService`, factored out into
`MeshTargets.h`/`ChannelLookup.h/.cpp` so both agree).

`isChargeComplete()`'s `STBY` read was flipped from the datasheet-assumed
active-low to active-HIGH per explicit instruction, superseding the earlier
unconfirmed assumption (`board/README.md` "Open questions"). `isCharging()`'s
`CHG` read is unchanged (still active-low).

- Build succeeded clean (only pre-existing upstream warnings, none from LIA
  code) and flashed successfully to COM4.
- **Confirmed no regression on real hardware**: a 45s post-flash serial
  capture shows a clean boot with no crash, `TrackerService` running
  normally (`no GPS fix yet` at uptime 31s and 61s, on schedule), NodeInfo
  and DeviceTelemetry sending normally -- `ChargeStatusService` present and
  not erroring, simply hadn't seen a state transition in that window (its
  edge-triggered design only sends on change, so this alone doesn't confirm
  polarity).
- **Charging/charge-complete messages confirmed correct by the user**
  (2026-07-22) after a live physical test on real hardware -- both the
  `"Charging"` and `"Device charged"` messages arrived at the target node as
  expected.
- **`STBY` polarity correction, later the same day**: the user provided the
  actual TP4056/board wiring detail -- `STBY` is pulled HIGH by an external
  resistor while charging and pulled LOW by an internal N-MOSFET once
  charging completes (mirroring `CHG`'s own active-low-while-charging
  behavior, just on the opposite event). This means the active-HIGH read
  above was backwards despite passing the live test (likely because that
  test's charge-complete state happened to coincide with conditions where
  either polarity would have produced a plausible-looking result). Flipped
  `isChargeComplete()` back to `STBY == LOW` to match the real wiring.
- **RED LED polarity bug found and fixed** (2026-07-22), also surfaced during
  this phase's live testing: `LiaBoard::setRedLed()` assumed common-anode
  wiring (channel lights when driven LOW) and inverted its duty cycle
  accordingly, but real hardware showed the LED ON while BMS read LOW (which
  calls `setRedLed(0)`, intending OFF). `TrackerService`'s BMS branching
  itself was already independently confirmed correct (send cadence, sleep
  triggering), isolating the bug to the LED driver's polarity assumption, not
  the BMS read. Fixed by removing the inversion; user confirmed both LED
  states correct after reflashing (solid ON in continuous/BMS-HIGH, OFF in
  sleep-cycle/BMS-LOW).

## Phase 8 notes

RTC wake, radio sleep, and GPS power-down were already confirmed in Phase 6.
Added BMS wake and USB wake as additional deep-sleep wake sources: a
`variant_shutdown()` override (weak-overridden per
`src/platform/esp32/main-esp32.cpp`'s `cpuDeepSleep()`, called immediately
before it configures RTC-domain wake sources and calls
`esp_deep_sleep_start()`) arms `esp_sleep_enable_ext0_wakeup(LIA_PIN_CHG,
LOW)` and `esp_sleep_enable_ext1_wakeup(1ULL << LIA_PIN_BMS,
ESP_EXT1_WAKEUP_ANY_HIGH)`.

Both GPIO1 (CHG) and GPIO15 (BMS) are RTC-capable on ESP32-S3 (confirmed via
`rtc_io_channel.h`: `RTCIO_GPIO1_CHANNEL`/`RTCIO_GPIO15_CHANNEL` both exist,
`SOC_RTCIO_PIN_COUNT` is 22, i.e. GPIO0-21). ext0 is a single-pin/single-level
source and ext1 applies one shared level across its whole bitmask -- since
CHG needs LOW and BMS needs HIGH, they can't share one registration, so each
pin gets its own independent HW wake source (ext0 for CHG, ext1 for BMS)
instead. There's no separate VBUS-sense pin on this board, so CHG (asserted
whenever the TP4056 is actively charging the battery) is the closest
available proxy for "USB connected."

- Build succeeded clean and flashed successfully to COM4.
- **Confirmed no regression on real hardware**: a 40s post-flash capture
  shows a clean boot, no crash, `TrackerService` running normally.
- **Confirmed real deep sleep still enters cleanly** with `variant_shutdown()`
  now wired in: switching BMS to LOW, the board waited the full
  `kFixWaitTimeoutMs` (no fix indoors), then `TrackerService: entering deep
  sleep for 60s (BMS low)`, graceful SX1262/GPS shutdown, prefs saved, no
  crash or assert -- same clean sequence as Phase 6's already-verified path,
  confirming the new wake-source registration doesn't interfere with normal
  sleep entry.
- **BMS wake / USB wake early-wake timing not yet directly captured**: no
  serial capture yet shows a wake happening *before* the 60s RTC timer would
  have fired anyway (the wake source that actually fired isn't distinguished
  in this round of testing). Needs a timed test -- flip BMS back to HIGH (or
  toggle CHG) a few seconds into the sleep window and confirm the reported
  wake cause (`ext0 RTC_IO` vs `ext1 RTC_CNTL` vs `timer`, per
  `sleep.cpp::initDeepSleep()`'s logging) and that it happens well under 60s.

## lia_v1 I2C rework (2026-07-22)

A solder rework crossing SDA/SCL back on the LSM6DSOXTR IMU's connection
fixed the original wiring defect that made the bus unusable (see
`board/README.md` "Open questions"). `variant.h` now defines `I2C_SDA`/
`I2C_SCL` (GPIO4/5) directly on `lia_v1` (this is a physical rework to the
existing board, not a new PCB revision like `lia_v2`) -- both the MAX17048
battery gauge and the IMU should now be usable, with no LIA-specific code
required (same mechanism as `lia_v2`, see `services/README.md`).

- Build succeeded clean and flashed successfully to COM4.
- Getting a clean capture of the boot-time I2C scan took several attempts:
  this board's USB-Serial-JTAG interface doesn't become openable until
  roughly 15s into boot, well after the scan already ran and scrolled by
  unbuffered. A plain reflash-then-capture raced this reliably on the
  `lia_v2` board earlier but not consistently here; what finally worked was
  a retry-loop script that kept re-opening the port across a real physical
  USB unplug/replug until the read succeeded (`serial_capture_robust.py`,
  scratchpad-only, not part of the repo).
- **Confirmed on real hardware**: clean boot, no crash, and the I2C scan
  shows both devices:
  ```
  Scan for i2c devices
  MAX17048 found at address 0x36
  Register value from 0x6b: 0x6c
  QMI8658 found at address 0x6b
  2 I2C devices found
  ```
  The IMU is logged as "QMI8658" rather than "LSM6DSOXTR"/"LSM6DS3" --
  0x6C is the LSM6DSOX family's actual `WHO_AM_I` chip-ID value, and
  `ScanI2CTwoWire` apparently resolves that ID/address combination to the
  QMI8658 label rather than the LSM6DS3 one it uses elsewhere (both are
  common register-compatible 6-axis IMUs at this address) -- the physical
  part is still the LSM6DSOXTR, just surfaced under Meshtastic's own device
  labelling for that chip ID.
- Not yet done: reading either sensor's actual telemetry output (gauge
  voltage/percent, IMU accel/gyro) from the mesh to confirm real data, not
  just presence detection.
