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
| `setRedLed(uint8_t)` | `LIA_PIN_LED_RED` (GPIO40) | Common-anode PWM, 0 = off / 255 = full brightness. |
| `isCharging()` | `LIA_PIN_CHG` (GPIO1) | TP4056 `CHRG`, open-drain active-low. |
| `isChargeComplete()` | `LIA_PIN_STBY` (GPIO2) | TP4056 `STBY`, open-drain active-low. |
| `isBmsHigh()` | `LIA_PIN_BMS` (GPIO15) | Raw mode-switch reading -- see [Open questions](#open-questions). |

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
  `notifyDeepSleep` observable starting Phase 6/8, once there's an actual
  sleep policy to hook it to.

## Open questions

- **BMS polarity.** `firmware/AGENTS.md` (Phase 6) specifies BMS HIGH as the
  continuous/no-sleep/LED-on behaviour and BMS LOW as the wake-every-minute
  sleep behaviour. The original project brief (`MOD.md`) names these the
  other way around ("BMS high = Tracker mode" = the sleepy behaviour). Rather
  than guess, `LiaBoard` only exposes the raw `isBmsHigh()` reading -- confirm
  which switch position the user expects for which behaviour before wiring
  Phase 6's tracker/beacon state machine to it.
- **Charger status polarity.** `isCharging()`/`isChargeComplete()` assume the
  TP4056's `CHRG`/`STBY` are open-drain active-low (per its datasheet), read
  with the ESP32's internal pull-ups enabled. Confirm against real hardware
  in Phase 7.
