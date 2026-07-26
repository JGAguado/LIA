---
title: Development History
description: A phase-by-phase log of how LIA's firmware got built, including the bugs found along the way.
---

This is a running, chronological log of LIA's firmware development — written like a blog rather than a spec, because a lot of what's interesting here is *what went wrong and why*, not just what shipped. The formal phase roadmap this follows lives in [`firmware/AGENTS.md`](https://github.com/JGAguado/LIA/blob/main/firmware/AGENTS.md); the day-to-day validation notes behind each entry live in [`firmware/docs/phase-status.md`](https://github.com/JGAguado/LIA/blob/main/firmware/docs/phase-status.md).

## Phase 0-1 — Board bring-up and peripheral power

The first firmware milestone was the boring-but-essential one: a PlatformIO project for the `lia_v1` board variant that builds, flashes, and boots to a working USB serial console. Peripheral power control (`PPC`) came next — the SX1262 radio and the SAM-M10Q GPS module share a power rail, gated by a single GPIO, and both need it driven high before anything talks to them.

## Phase 2 — Getting the radio talking (and two real crashes)

Turning the board into an actual Meshtastic node — SPI to the SX1262, a periodic "LIA Radio Test" broadcast — hit two genuine firmware bugs on the very first flash, both root-caused with `addr2line` against the built ELF rather than guessing from log output alone:

1. A null-pointer dereference: the test module's periodic callback read a global (`airTime`) that Meshtastic's own `main.cpp` hadn't constructed yet at the point our hook ran.
2. A more subtle one: a single `LOG_INFO()` call placed too early in boot (before the console/logging subsystem itself was initialized) crashed inside Meshtastic's own logging code. The fix was just moving that log line later — but finding *why* a seemingly-harmless print statement crashed the board took real investigation.

Once fixed, the destination node received "LIA Radio Test" for real, confirmed independently from the receiving side via the official Meshtastic app.

## Phase 3.5-5 — TrackerService and real GPS positions

The ad-hoc test sender got formalized into `TrackerService`, then upgraded to send actual GPS positions instead of a fixed string once the SAM-M10Q was confirmed working (it turned out to need no custom code at all — stock Meshtastic already auto-detects and drives it). Verified against a real 39-node mesh with genuine RF round-trips, not just local logs.

## Phase 6 — The BMS switch, and a deep-sleep crash

This is where the physical mode switch (`BMS`) got wired up: switch HIGH means continuous tracking (LED on, broadcast every 30 seconds, never sleep); switch LOW means a low-power cycle (wake roughly once a minute, get a fix, send it, sleep again).

Getting the sleep half of this working on real hardware surfaced a genuine crash: deep sleep uses an observer pattern where multiple parts of the firmware register interest in "we're about to sleep, do your cleanup." LIA's own power-gating hook was registering *too early* — before the radio and GPS drivers had registered their own graceful-shutdown hooks — so peripheral power was being cut before those drivers could tell their chips to standby cleanly. The radio's own internal assertion caught the resulting SPI failure. The fix was purely about *when* to register the hook, not what it does; confirmed across two full real sleep/wake cycles afterward.

## Standalone GPS test, and a private channel for position

Once the tracker was sending real positions, the next question was privacy: Meshtastic encrypts per-channel rather than per-recipient, and position handling is promiscuous by design (any node that can decrypt a packet updates its own map from it, regardless of who it was addressed to). Sending on the default public channel meant *any* node on the mesh could see this device's location. The fix was to send only on a private channel — named `Test`, looked up dynamically by name rather than a hardcoded index — so only people who'd been given that channel's key could see it. This same channel became the backbone for everything LIA-specific added afterward.

Around the same time, a small standalone PlatformIO project (`firmware/tools/gps_test/`) — no Meshtastic, just TinyGPSPlus and a serial print every 10 seconds — was built to get an outdoor cold-fix confirmation independent of the rest of the firmware's complexity: a real fix in about 36 seconds, 10 satellites.

## Phase 7 — Charging status, and a wiring lesson

The original plan for charge status was a breathing/solid/off LED pattern. That got superseded by a more useful design: text messages (`"Charging"`, `"Device charged"`) sent to the private channel instead, since the LED was already spoken for by the BMS indicator.

The `STBY` pin's polarity (which level means "charge complete") went through a real correction here. An initial assumption had it backwards; a live hardware test happened to *look* like it passed anyway, which is exactly the kind of false confidence a single test can give you. It was the actual TP4056 charge-controller wiring — an external pull-up holds `STBY` high while charging, an internal switch pulls it low on completion — that settled it for good, once that detail was worked out precisely.

## Phase 8 — Waking early instead of waiting

The low-power sleep cycle originally only woke on its own once-a-minute timer. Phase 8 added two more ways to wake immediately instead of waiting out the interval: flipping the BMS switch, or plugging in a charger — both wired as real hardware interrupts (ESP32 RTC-domain wake sources), armed right before every deep sleep.

## An LED polarity bug, found by testing the thing it was supposed to fix

While confirming Phase 7 on real hardware, the RED LED's actual behavior didn't match its intended behavior — inverted, as if the wiring assumption behind the LED driver code was backwards. Since the BMS state machine driving it had already been independently proven correct (by its send/sleep timing, unrelated to the LED at all), the bug was isolated cleanly to the LED driver's own polarity assumption. Fixed by removing an unnecessary inversion, confirmed by eye afterward.

## The wrong LED part

The board's status indicator was designed around a true RGB LED (LCSC part `C397052`), but the first assembled batch has a different part soldered in its place: a two-color Red/Green LED (`C601680`), sharing the same 4-pin footprint but not the same die. Of that part's two colors, only Red ended up wired to a firmware-driven channel, so the assembled units only ever show RED regardless of what the design intends the LED to be capable of. This is a sourcing/assembly mistake to correct in a future PCB spin, not a firmware bug.

## A second hardware revision, and a rework that unified them

A second physical board arrived with its accelerometer IC (an LSM6DSOXTR, sharing the I2C bus with a MAX17048 battery gauge) desoldered entirely — the original board's `SDA`/`SCL` lines to that chip had been crossed as manufactured, which caused I2C bus errors badly enough that I2C had to be left disabled on the original board altogether. Removing the IMU sidestepped the problem rather than fixing it, freeing the bus for the gauge alone. This became `lia_v2`, a second PlatformIO board variant sharing all the same LIA-specific firmware — the only difference is which pins get defined for I2C.

Later, the original board got a solder rework crossing `SDA`/`SCL` back correctly, fixing the wiring defect at its source rather than working around it. With that rework applied, `lia_v1` also got I2C enabled, and — since the LSM6DSOXTR/MAX17048 chips report a chip-ID collision that Meshtastic's own detection labels under a different sensor name (`QMI8658`) than what stock Meshtastic actually knows how to drive — a small dedicated driver (`ImuMotionDriver`) was added to talk to the real chip directly rather than relying on stock support that silently doesn't apply here. Both boards ended up physically identical shortly after, once the same rework was applied to the second board too — at which point `lia_v2` as a separate firmware variant no longer served any purpose and was retired; a single `lia_v1` build now covers both, and the hardware-only battery/enclosure distinction between them is tracked instead as V1.0/V1.1 (see [Battery](/LIA/docs/firmware/battery/) and [Enclosure](/LIA/docs/hardware/enclosure/)).

## CommandService — a text-command interface, and two bugs it surfaced

With both boards now capable of reading the battery gauge and (on `lia_v1`) the IMU, a text-command interface (`GPS`, `BATTERY`, `IMU ON`/`OFF`, `CHG ON`/`OFF`, `STB ON`/`OFF`, `HELP`, later `LED ON`/`OFF`) replaced one-way notifications with something interactive. Building and testing it on real hardware surfaced two more real bugs:

- **Direct messages were being silently dropped.** The private-channel design above worked fine for broadcasts, but a direct message sent through the official app is commonly PKI-encrypted — a mechanism that bypasses the channel/PSK system entirely and reports a channel value that doesn't mean what it looks like it means. The fix: also accept any message addressed directly to the device, regardless of that value, since being addressed directly already requires knowing the device in the first place.
- **The battery gauge always reported ~0%,** even after fixing an initial suspicion (that reading the gauge right after resetting it was catching it before it settled). A standalone, Meshtastic-free test project (`firmware/tools/battery_test/`) built specifically to isolate this compared two independent read paths side by side — they agreed with each other exactly, and neither improved over time, which ruled out the reset theory. What they agreed on was a real, low cell voltage, independently confirmed with a multimeter. Not a bug at all — a genuinely low battery.

The command syntax itself iterated a couple of times in response to how people actually typed commands — from underscores (`LED_ON`) to no separator (`LEDON`) to, finally, a plain space (`LED ON`), after real usage showed people naturally typing "Led off" and expecting it to just work.

## Dropping the private channel for direct addressing

The private-channel design worked, but it asked for something people kept getting wrong in practice: a specifically-named channel (`Test`) had to exist and be shared out-of-band before anything LIA-specific would do anything at all, and getting that wrong meant messages silently failing closed with no obvious signal why. Given that direct messages were already being special-cased to work around PKI encryption (see above), the channel lookup was doing less and less real work — a DM to a specific node already requires knowing/pairing with that node, and the official app's PKI encryption to that node's identity key isn't a weaker bar than possessing a channel's PSK. The channel concept (and its supporting `ChannelLookup` lookup-by-name helper) was removed entirely in favor of addressing everything — position broadcasts, charge-status messages, and all `CommandService` traffic — as a direct message to a single predefined target node (`MeshTargets.h`'s `kLiaTargetNode`, derived from that node's MAC address).

## What's next

See the [Roadmap](https://github.com/JGAguado/LIA#roadmap) in the main README for where things stand at a glance, and [`firmware/docs/phase-status.md`](https://github.com/JGAguado/LIA/blob/main/firmware/docs/phase-status.md) for the detailed, evidence-quoted version of everything above.
