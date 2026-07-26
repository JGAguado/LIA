# Services

## Battery gauge + IMU (I2C)

`variant.h` defines `I2C_SDA`/`I2C_SCL`, which is enough for stock
Meshtastic's `Wire.begin(I2C_SDA, I2C_SCL)` + `i2cScanner->scanPort()`
(`src/main.cpp`) to detect both parts at boot -- confirmed via the boot log
(`MAX17048 found at address 0x36`, `QMI8658 found at address 0x6b`, `2 I2C
devices found`). This only works because of a solder rework crossing
SDA/SCL back on the IMU's connection (2026-07-22) -- see `board/README.md`
"Open questions" for why the bus was unusable before that.

Detection stops there for the IMU, though: `ScanI2CTwoWire` resolves the
LSM6DSOXTR's `WHO_AM_I` (0x6C) to its `QMI8658` device type, and
`AccelerometerThread` (`motion/AccelerometerThread.h`) has no `case` for
that type at all -- it silently falls through to `disable()`. So unlike the
gauge, stock Meshtastic never actually drives this IMU. `CommandService`'s
`ImuMotionDriver` (`drivers/ImuMotionDriver.*`) talks to it directly instead,
via `Adafruit_LSM6DSOX` (0x6C matches that class's expected chip ID exactly,
unlike the `Adafruit_LSM6DS3TRC` class Meshtastic's own `LSM6DS3Sensor.cpp`
uses for a different chip ID).

Similarly, `CommandService`'s `BATTERY` command reads the `Adafruit_MAX1704X`
gauge directly rather than through `powerStatus->getBatteryChargePercent()`:
`Power::setup()` runs its own battery-source detection *before* `main.cpp`'s
I2C scan populates the sensor map that detection depends on, so it always
sees "not ready yet" at boot and locks in a 101% ("no battery, external
power") fallback that never gets corrected later -- confirmed on real
hardware (2026-07-22), even though the gauge is genuinely present and
working.

## MeshTargets.h

Shared configuration used by every service that reports to the configured
target node: `kLiaTargetNode` (NodeNum, per `firmware/AGENTS.md` "Target
Node") lives in `MeshTargets.h` -- the one place it's defined, so
`TrackerService`, `ChargeStatusService`, and `CommandService` can't drift
apart. Everything LIA-specific is a direct message (`p->to = kLiaTargetNode`)
to this node, never a channel broadcast -- per explicit instruction
(2026-07-26), superseding an earlier design that routed everything through a
channel found by name (`kLiaChannelName`/`ChannelLookup.h/.cpp`, both
removed). That design worked but required out-of-band channel setup and
failed closed silently when misconfigured; a direct message already
requires knowing/pairing with the target node and is commonly
PKI-encrypted by the official app to that node's identity key, so it isn't
a weaker bar than possessing a channel's PSK.

## TrackerService

Owns periodic mesh transmissions and the tracker/beacon state machine, per
`firmware/AGENTS.md`'s architecture. Sends the current GPS position
(`meshtastic_Position`, port `POSITION_APP`) as a unicast, built from the
global `localPosition` (`NodeDB.h`) the same way stock `PositionModule.cpp`
does -- skips the send if there's no fix yet (lat/lon both zero) rather than
sending a meaningless (0,0). The destination is `kLiaTargetNode`
(`MeshTargets.h`).

### Direct-message targeting

Position packets are sent as a direct message to `kLiaTargetNode`, never a
broadcast on any channel. Meshtastic encrypts per-channel (not
per-recipient) and `PositionModule` processes any overheard position packet
promiscuously regardless of its `to` field, so broadcasting on any channel
would let *any* node holding that channel's PSK map this device -- a direct
message to a specific node, commonly PKI-encrypted by the official
app/firmware, restricts visibility to that one node instead, per explicit
instruction (2026-07-26; see "MeshTargets.h" above for why this replaced an
earlier private-channel design).

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

### Manual LED override

`setManualLed(bool on)` (reached via `TrackerService::instance()`, used by
`CommandService`'s `LED ON`/`LED OFF`) sets the LED immediately and a
one-way flag that makes `runOnce()` stop driving it from BMS state from then
on. There is no "back to automatic" command -- a fresh boot (e.g. after the
BMS-LOW sleep cycle's reboot) starts back in automatic mode, since the flag
isn't persisted.

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

Reports `LiaBoard`'s charger status to `kLiaTargetNode` as a direct message
(`TEXT_MESSAGE_APP`) -- Phase 7, per explicit instruction (2026-07-22) to
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

## CommandService

Responds to short text commands, per explicit instruction (2026-07-22).

Accepts a command only if it arrives as a direct message addressed to us
specifically (`mp.to == nodeDB->getNodeNum()`) -- per explicit instruction
(2026-07-26), superseding an earlier design that also (or instead) gated on
a private channel found by name. A DM sent through the official app is
commonly PKI-encrypted, which bypasses the channel/PSK system entirely and
reports `channel=0` no matter which channel was open when it was sent --
confirmed on real hardware (2026-07-22). Being addressed directly to our
node already requires the sender to know/have paired with us, and PKI
authenticates to their actual identity key, so this isn't a weaker bar than
channel-PSK possession.

Commands (case-insensitive and whitespace-normalized -- extra/mixed spaces
or tabs between the two words of an ON/OFF command still match, e.g. "led
 off" or "Led  Off" both match `LED OFF`; unrecognized text is silently
ignored, since a DM to this device may carry normal chat too):

| Command | Behaviour |
| --- | --- |
| `GPS` | Replies with the current `localPosition` lat/lon, or "No GPS fix yet". |
| `BATTERY` | Replies with the MAX17048's charge percentage (see "Battery gauge + IMU" above for why this reads the gauge directly). |
| `IMU ON` | Starts `ImuMotionDriver` (begin()s the sensor + arms its GPIO interrupt on first use) and polls it every 250ms; sends "Activity detected" once per motion event as a direct message to `kLiaTargetNode` (same destination `TrackerService`/`ChargeStatusService` use, not a reply to whoever sent `IMU ON`). |
| `IMU OFF` | Detaches the interrupt; stops polling. |
| `CHG ON` / `CHG OFF` | Toggles `ChargeStatusService::instance()->setChargingNotificationsEnabled()`. |
| `STB ON` / `STB OFF` | Toggles `ChargeStatusService::instance()->setChargeCompleteNotificationsEnabled()`. |
| `LED ON` / `LED OFF` | Calls `TrackerService::instance()->setManualLed()` -- see `TrackerService`'s own doc below for the one-way-override caveat. |
| `HELP` | Replies with the command list. |

Commands originally used an underscore (`IMU_ON`) and were briefly
concatenated with no separator (`IMUON`) before settling on a space, per
explicit instruction (2026-07-24) -- real usage showed people naturally
type "Led off" with a space.

Replies to a command go back as a direct message to whoever sent it
(`isPromiscuous = true` so the module sees all traffic on its port, but only
messages addressed directly to us are acted on -- see above).

`ChargeStatusService::instance()`/`TrackerService::instance()` are
self-registering static pointers set in each class's own constructor (not a
lazily-constructed Meyer's singleton like `LiaBoard`, since their
construction *timing* matters -- both must happen from `lateInitVariant()`,
in a specific order (`TrackerService`, then `ChargeStatusService`, then
`CommandService`) -- the same pattern Meshtastic's own globals (`nodeDB`,
`service`, `screen`, ...) already use.

Constructed once from `lateInitVariant()`, after `TrackerService` and
`ChargeStatusService` (`new CommandService();`) -- no global pointer kept,
nothing else needs to reach this one.

### Battery gauge reading confirmed correct (2026-07-24)

`BATTERY` kept reporting ~0-1% even after the no-reset fix above. Built a
standalone `firmware/tools/battery_test/` project (no Meshtastic -- see its
README) to compare the no-reset raw register read against the stock
`Adafruit_MAX17048` library side by side, isolated from any
Meshtastic-specific interaction. Both paths agreed exactly and neither
improved over ~28s of samples (ruling out "just needs time to reconverge
after reset"), while `VCELL` read a stable ~2.95-2.96V -- confirmed
independently with a multimeter at ~3.0V, within normal measurement
tolerance. A LiPo cell at ~3.0V is close to a typical low-voltage cutoff, so
a near-0% state of charge is a genuinely correct reading, not a bug -- no
further code change was needed.
