# Battery gauge bring-up test

Standalone MAX17048 test for LIA v1 -- no Meshtastic, no LoRa, no GPS.
Isolates a real-hardware bug where `CommandService`'s `BATTERY` command
always reports ~0% regardless of actual charge state (see
`firmware/docs/phase-status.md`). Deliberately independent of
`firmware/meshtastic/` -- this is a plain Arduino sketch, not a Meshtastic
build, so it has no dependency on a Meshtastic checkout.

Prints two independently-computed readings side by side every 2s over
native-USB serial (115200 baud):

- **`[raw]`** -- a direct SOC/VCELL register read via `Adafruit_I2CDevice` +
  `Adafruit_BusIO_Register`, never calling `Adafruit_MAX17048::begin()`
  (which sends the chip a hardware reset command, `0x5400`, before every
  use). Same approach `CommandService::batteryPercent()` uses.
- **`[library]`** -- the stock `Adafruit_MAX17048` class, `begin()`'d once at
  setup (so its one-time reset happens before any readings are taken, same
  as it would in Meshtastic).

## Build & flash

From this directory (`firmware/tools/battery_test/`), with the board
connected:

```powershell
pio run -e lia_v1 -t upload --upload-port <PORT>
```

## Reading the output

```
Adafruit_MAX17048::begin() -> OK
Chip ID: 0x..., IC version: 0x...
t=2s [raw] SOC=xx.xx% VCELL=x.xxxxV  [library] SOC=xx.xx% VCELL=x.xxxxV
t=4s [raw] SOC=xx.xx% VCELL=x.xxxxV  [library] SOC=xx.xx% VCELL=x.xxxxV
```

- `Adafruit_MAX17048::begin() -> FAILED`, or no `Chip ID`/`IC version` line
  -- the gauge isn't responding on I2C at all (wiring, address, or power
  rail problem). Neither `[raw]` nor `[library]` can be trusted.
- `VCELL` reading a plausible battery voltage (roughly 3.0-4.2V for a LiPo)
  while `SOC` stays near 0% -- the chip sees a real battery but its
  charge-percentage algorithm hasn't produced a sensible estimate. If
  `[library]`'s `SOC` climbs toward a plausible value over the first
  samples while `[raw]` does not, that confirms the reset-then-immediate-read
  theory (the fix already applied to `CommandService`: read without
  resetting, and expect it to already be correct from the first sample).
  If **both** stay stuck near 0% indefinitely, the reset-timing theory is
  wrong and the real cause is something else (needs `quickStart()`, a
  hibernation/config register in a bad state, or the chip has never been
  calibrated against this specific cell).
- `VCELL` reading ~0V too -- not a percentage-calculation problem at all;
  check the physical battery connection to the gauge's sense pins.
