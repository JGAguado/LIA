---
title: Battery
description: Battery selection, charging, and gauge reading on LIA.
---

LIA comes in two battery configurations, tied to the board/enclosure revision (see [Enclosure](/LIA/docs/hardware/enclosure/)) — this is a **hardware-only** distinction, both revisions run the identical `lia_v1` firmware (see [Building the Firmware](/LIA/docs/firmware/building-the-firmware/#which-variant)):

- **V1.0** boards pair with a single 18650 Li-ion cell, held in a Keystone 54 clip on the main board (see [BOM](/LIA/docs/hardware/bom/)) — 3500 mAh.
- **V1.1** boards pair with a 50×37×4 mm pouch cell — 1000 mAh, in exchange for a much more compact enclosure.

## Charging

Charging is exposed through a magnetic 4-pin pogo connector (`J2`), so the enclosure never needs to be opened to charge the device — a standard USB-C-to-magnetic-pogo cable is used at the collar side.

## Protection

The cell does **not** need a built-in protection circuit — over-discharge, over-current, and short-circuit protection are handled on the board itself by a TP4056 charge controller (see the `CHG`/`STBY` status pins below, and the protection FETs in the [BOM](/LIA/docs/hardware/bom/)). A plain, unprotected cell is fine.

### Charge status messages

Rather than a status LED pattern, LIA reports charge state as a direct message to the configured target node (see [Meshtastic Configuration](/LIA/docs/firmware/meshtastic-configuration/)):

- **`"Charging"`** once, when a charger is first detected (`CHG` pin, active-low).
- **`"Device charged"`** once, when the charge cycle completes (`STBY` pin — pulled HIGH by an external resistor while charging, pulled LOW by an internal N-MOSFET on completion).

Send `CHG OFF` / `STB OFF` as a direct message to the device to mute either message (or `CHG ON` / `STB ON` to re-enable) — see [Meshtastic Configuration](/LIA/docs/firmware/meshtastic-configuration/) for the full command list.

## Battery gauge

State of charge is read from a **MAX17048G+T10** fuel gauge IC, which tracks cell voltage and estimates remaining capacity without needing a current-sense shunt.

Send `BATTERY` as a direct message to the device to get the current percentage on demand.

:::note
This reads the gauge directly rather than through Meshtastic's own battery-percentage reporting: `Power::setup()` (stock Meshtastic) runs its own gauge detection *before* the I2C bus scan that would actually find the MAX17048, so it always sees "not ready yet" at boot and falls back to reporting "no battery" indefinitely — a real bug found via a standalone, Meshtastic-free test project built specifically to isolate it (`firmware/tools/battery_test/`), which confirmed the gauge itself reads correctly (its voltage measurement matched an independent multimeter reading) once read the right way.
:::

## Runtime

See [Power Management](/LIA/docs/firmware/power-management/) for the sleep/wake model that determines actual runtime. Measured runtime numbers from real hardware will be published here once available.

See also: [Power Management](/LIA/docs/firmware/power-management/), [Accelerometer](/LIA/docs/firmware/accelerometer/), and [Microcontroller](/LIA/docs/firmware/microcontroller/).
