# LiaBoard

Hardware abstraction for the peripherals on LIA's board that stock Meshtastic
has no concept of. Sits between the `lia_v1` board variant and the rest of
the firmware, per `firmware/AGENTS.md`'s architecture:

```
Meshtastic Core -> Board Definition -> LiaBoard -> Drivers -> TrackerService
```

## What it owns

| Method | Backing pin | Notes |
| --- | --- | --- |
| `enablePeripherals()` / `disablePeripherals()` | `LIA_PIN_PPC` (GPIO21) | Gates power to the SX1262 and SAM-M10Q. Must be enabled before either is touched. |
| `setRedLed(uint8_t)` | `LIA_PIN_LED_RED` (GPIO40) | Active-high PWM, 0 = off / 255 = full brightness (see "Open questions"). |
| `isCharging()` | `LIA_PIN_CHG` (GPIO1) | TP4056 `CHRG`, open-drain active-low. |
| `isChargeComplete()` | `LIA_PIN_STBY` (GPIO2) | True when `STBY` reads HIGH -- per explicit instruction (2026-07-22), see "Open questions" below. |
| `isBmsHigh()` | `LIA_PIN_BMS` (GPIO15) | Mode-switch reading. HIGH = continuous/no-sleep, LOW = wake-every-minute sleep cycle -- see `TrackerService` (Phase 6). |

It deliberately does **not** touch the SX1262 or SAM-M10Q themselves (that's
stock Meshtastic, driven by the pin `#define`s in
[`variant.h`](../meshtastic/variants/esp32s3/lia_v1/variant.h)), and never
touches I2C or the green/blue LED channels -- both are unavailable on the
current PCB revision (see `firmware/AGENTS.md` "Hardware Constraints").

## Wiring into Meshtastic

`LiaBoard` is a Meyer's singleton (`LiaBoard::instance()`) rather than a
global, per the "No globals" coding standard. It's driven from
[`extra_variants/lia_v1/variant.cpp`](../meshtastic/extra_variants/lia_v1/variant.cpp):

- `earlyInitVariant()` calls `begin()` then `enablePeripherals()`. This runs
  *before* `initSPI()` and before Meshtastic's own radio/GPS init (which
  happens later, inside `setupModules()`) -- PPC must already be high by
  then, so this cannot be done from `lateInitVariant()` (confirmed against
  `src/main.cpp` and `src/platform/extra_variants/README.md` in the
  Meshtastic checkout: `lateInitVariant()` runs *after* the LoRa radio is
  initialized).
- Deep-sleep power-down (`disablePeripherals()`) is wired to Meshtastic's
  `notifyDeepSleep` observable (Phase 6). This fires for any deep sleep path
  system-wide, not just `TrackerService`'s own `doDeepSleep()` calls --
  confirmed this actually runs for LIA: Meshtastic only *skips* calling
  `notifyDeepSleep` when `shouldLoraWake()` is true, which is only the case
  for `ROUTER`/`ROUTER_LATE` roles (`sleep.cpp`), not `TRACKER`.
  **Registration point matters**: `RadioInterface::init()` and
  `GPS::setup()` *each also* register their own `notifyDeepSleep` observer
  (to command the SX1262/GNSS to standby gracefully over SPI/UART while
  still powered), both before `lateInitVariant()` runs. `Observable` fires
  observers in registration order, so `armDeepSleepHook()` (which does the
  actual `.observe()` call) must be called from `lateInitVariant()`, *not*
  from `begin()`/`earlyInitVariant()` -- registering earlier cuts PPC before
  those graceful commands can reach the hardware. Confirmed by a real crash
  (`SX126xInterface::setStandby()` failing with `RadioLib err=-707` once PPC
  had already gone low first) before this was fixed.

## Open questions

- ~~**RED LED polarity.**~~ Resolved 2026-07-22: `setRedLed()` was assumed
  common-anode (channel lights when driven LOW) and inverted its duty cycle
  accordingly. Real hardware showed the LED ON while BMS read LOW (which
  calls `setRedLed(0)`, intending OFF) -- the channel actually lights when
  driven HIGH, so the inversion was removed. Confirmed against
  `TrackerService`'s already-independently-confirmed BMS branching (send
  cadence, sleep triggering), so the mismatch was isolated to the LED driver,
  not the BMS read.
- ~~**BMS polarity.**~~ Resolved 2026-07-19: follow `firmware/AGENTS.md`
  (Phase 6) as the authoritative source -- BMS HIGH is continuous/no-sleep,
  BMS LOW is the wake-every-minute sleep cycle. `MOD.md`'s opposite polarity
  ("BMS high = Tracker mode" = the sleepy behaviour) is not used.
- **Charger status polarity.** `isCharging()` still assumes the TP4056's
  `CHRG` is open-drain active-low (per its datasheet), read with the ESP32's
  internal pull-up enabled. `isChargeComplete()`'s `STBY` polarity was
  flipped to active-HIGH 2026-07-22 per explicit instruction, overriding the
  earlier active-low datasheet assumption -- Phase 7's real-hardware charge
  cycle is what actually confirms (or corrects) both of these.
- **GPS fix-wait timeout for the BMS-low sleep cycle** (`TrackerService.h`'s
  `kFixWaitTimeoutMs`, currently 90s) is a real tradeoff (battery vs. fresh
  position) not yet validated against actual cold/warm-fix timing outdoors --
  see the website's power-management.md "GPS fix timeout" note.
