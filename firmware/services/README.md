# Services

## TrackerService

Owns periodic mesh transmissions and the tracker/beacon state machine, per
`firmware/AGENTS.md`'s architecture. Sends the current GPS position
(`meshtastic_Position`, port `POSITION_APP`) as a unicast, built from the
global `localPosition` (`NodeDB.h`) the same way stock `PositionModule.cpp`
does -- skips the send if there's no fix yet (lat/lon both zero) rather than
sending a meaningless (0,0). The destination NodeNum is the single
`kDestination` constant in `TrackerService.h`, per `firmware/AGENTS.md`
"Target Node".

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
