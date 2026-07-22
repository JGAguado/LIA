# Services

## Battery gauge (lia_v2 only)

No LIA-specific service needed: `lia_v2/variant.h` defines `I2C_SDA`/
`I2C_SCL`, which is all stock Meshtastic requires to enable the MAX17048
battery gauge -- `src/main.cpp` calls `Wire.begin(I2C_SDA, I2C_SCL)` and runs
`i2cScanner->scanPort()` whenever `I2C_SDA` is defined; `ScanI2CTwoWire.cpp`
already recognizes the MAX17048 at its fixed I2C address and registers
`MAX17048Sensor`, which then reports battery voltage/percentage through
Meshtastic's normal `PowerTelemetry`/`DeviceTelemetry` path -- the same
mechanism any stock Meshtastic board with this gauge uses. `lia_v1` cannot
do this: the LSM6DSOXTR IMU that's present there caused signal errors on the
shared I2C line, so `lia_v1/variant.h` deliberately leaves `I2C_SDA`/
`I2C_SCL` undefined (see `board/README.md`).

## MeshTargets.h / ChannelLookup

Shared configuration used by every service that reports to the configured
target node: `kLiaTargetNode` (NodeNum, per `firmware/AGENTS.md` "Target
Node") and `kLiaChannelName` (`"Test"`) live in `MeshTargets.h` -- the one
place either is defined, so `TrackerService` and `ChargeStatusService` can't
drift apart. `findLiaChannelIndexByName()` (`ChannelLookup.h/.cpp`) resolves
a channel name to its index (0-7) fresh on every call rather than caching it,
since the lookup is a cheap 8-entry scan and re-resolving means a runtime
channel reconfiguration (e.g. the user adding "Test" after first boot, or
moving it to a different slot) takes effect without a rebuild. Returns -1 if
no channel with that name is currently configured; every caller fails closed
on -1 (logs a warning, skips the send) rather than falling back to the
default channel, since that fallback would silently defeat the whole point
of restricting visibility to a named, private channel.

## TrackerService

Owns periodic mesh transmissions and the tracker/beacon state machine, per
`firmware/AGENTS.md`'s architecture. Sends the current GPS position
(`meshtastic_Position`, port `POSITION_APP`) as a unicast, built from the
global `localPosition` (`NodeDB.h`) the same way stock `PositionModule.cpp`
does -- skips the send if there's no fix yet (lat/lon both zero) rather than
sending a meaningless (0,0). The destination is `kLiaTargetNode`
(`MeshTargets.h`).

### Channel targeting

Position packets are sent on the channel named `kLiaChannelName` (`"Test"`),
never on the default/primary channel. Meshtastic encrypts per-channel (not
per-recipient) and `PositionModule` processes any overheard position packet
promiscuously regardless of its `to` field, so sending on the public default
channel would let *any* node on the mesh map this device -- sending on a
private, PSK-protected channel instead restricts visibility to other users
who have that same channel configured, per the user's request (2026-07-20).

Behaviour switches on `LiaBoard::instance().isBmsHigh()` (Phase 6):

- **HIGH ("continuous")**: RED LED solid on, send every 30s
  (`kContinuousIntervalMs`), never sleeps.
- **LOW ("sleep-cycle")**: send once, then deep sleep for a minute
  (`kSleepCycleMs`) via `doDeepSleep()`. `LiaBoard`'s `notifyDeepSleep`
  observer cuts PPC (both SX1262 and GPS power, shared rail) before power-off.
  The next wake is a full reboot -- construction and `runOnce()` start fresh,
  so no state survives between wakes except what's in `localPosition`
  (persisted by the GPS subsystem, not us). Waits up to `kFixWaitTimeoutMs`
  (90s) for a fix before giving up on the cycle and sleeping anyway, rather
  than hunting indefinitely and draining the battery.

Requires the device to be configured with `device.role = TRACKER` (not the
default) for the sleep behaviour to work as intended -- confirmed against
`PowerFSM.cpp` and `sleep.cpp`: stock Meshtastic only auto-adds its own
competing light-sleep timers for non-tracker roles ("sleep will be initiated
through the modules" for trackers), and `notifyDeepSleep` (which
`LiaBoard` needs to cut PPC) is only skipped for `ROUTER` roles. Setting the
role is a runtime app/CLI config change, not something this firmware hardcodes
(matching "configurable through the official Meshtastic application").

Phase history: Phase 2 introduced it inline in
`extra_variants/lia_v1/variant.cpp` as a fixed "LIA Radio Test" text payload;
Phase 3.5 moved it here as a proper `SinglePortModule` + `OSThread`; Phase 5
replaced the text payload with the real position; Phase 6 added the BMS
state machine and deep sleep.

Constructed once from `lateInitVariant()` (`new TrackerService();`) --
nothing else needs to reference the instance, so no global pointer is kept
(MeshModule/OSThread both self-register on construction).

## ChargeStatusService

Reports `LiaBoard`'s charger status to `kLiaTargetNode` as plain text
(`TEXT_MESSAGE_APP`), on the same private `kLiaChannelName` channel
`TrackerService` uses -- Phase 7, per explicit instruction (2026-07-22) to
send messages instead of the RED LED breathing/off behaviour
`firmware/AGENTS.md` originally specified for this phase (see
`firmware/AGENTS.md` "Phase 7").

Polls `LiaBoard::instance().isCharging()` / `isChargeComplete()` every 3s
(`kPollIntervalMs`) and sends edge-triggered, not on every poll: `"Charging"`
once when `isCharging()` goes false->true, `"Device charged"` once when
`isChargeComplete()` goes false->true. Neither message repeats while its
condition stays asserted.

`isChargeComplete()`'s `STBY` read is active-low (see `board/README.md`
"Open questions" for the actual pull-up/N-MOSFET wiring) -- it briefly went
through an incorrect active-HIGH flip earlier in this phase before being
corrected back against the real board behaviour.

Constructed once from `lateInitVariant()` (`new ChargeStatusService();`) --
same self-registration pattern as `TrackerService`, no global pointer kept.
