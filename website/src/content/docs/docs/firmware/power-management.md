---
title: Power Management
description: Deep sleep, wake cycles, the BMS switch, and estimating battery life.
---

LIA's firmware spends most of its time in deep sleep when tracking continuously isn't needed, waking only to acquire a GPS fix and broadcast it over the mesh. The tradeoff is straightforward: shorter sleep intervals mean fresher position data but shorter battery life.

## The BMS switch

The board has an accessible physical switch with **three** positions, though the firmware only ever sees two of them:

- **HIGH — continuous ("Beacon") mode.** The status LED stays solid RED, the device never sleeps, and it broadcasts its position every 30 seconds. Intended for short-term, active use (a walk, actively searching for the animal) — it drains the battery quickly compared to the alternative below.
- **LOW — sleep-cycle ("Tracker") mode.** The device wakes roughly once a minute, tries to get a fresh GPS fix (giving up after 90 seconds and sleeping anyway if none arrives, rather than draining the battery hunting indefinitely), sends a position if it got one, then cuts power to the radio and GPS and goes back into deep sleep. This is the low-power default for everyday wear.
- **OFF — a hardware power cutoff, not a firmware state.** This third position disables the regulator that powers the ESP32-S3 entirely, while still allowing the battery to charge. The firmware never sees this position: when the switch is OFF, the MCU has no power to run any firmware at all, so there's nothing for `isBmsHigh()` to read — it's not a third case the code branches on, it's the absence of the other two.

:::caution
The board will not power on from USB/pogo power if the switch is in the OFF position. The charging cable only feeds the TP4056 charge circuit, not the system power rail — it does not power the ESP32-S3. Make sure the switch is in HIGH or LOW before connecting to flash or use the device over serial; see [Building the Firmware](/LIA/docs/firmware/building-the-firmware/).
:::

Toggling between HIGH and LOW takes effect on the next wake without needing a reflash. Two additional wake sources besides the once-a-minute timer let the device react to the switch (or a charger) sooner rather than waiting out the full interval:

- Flipping the switch to HIGH while the device is mid-sleep wakes it immediately.
- Connecting a charger while the device is mid-sleep also wakes it immediately (there's no separate USB-presence pin on the board, so the charger-detect signal is used as the closest available proxy for "USB connected").

## Status LED

- In continuous mode, the status LED is solid RED by default, driven automatically from the switch state.
- Sending **`LED ON`** / **`LED OFF`** as a direct message to the device overrides this manually for the rest of that boot — see [RGB LED](/LIA/docs/firmware/rgb-led/) and [Meshtastic Configuration](/LIA/docs/firmware/meshtastic-configuration/). There's no "back to automatic" command; a fresh boot (e.g. the next sleep-cycle wake) resets to automatic control.

## Estimating battery life

A rough estimate for runtime $T$ (in hours) given a battery capacity $C$ (mAh), average current draw while awake $I_{awake}$ (mA), average sleep current $I_{sleep}$ (mA), duty cycle $d$ (fraction of time awake), and depth-of-discharge margin $m$:

$$
T = \frac{C \cdot m}{d \cdot I_{awake} + (1 - d) \cdot I_{sleep}}
$$

### Current budget

$I_{awake}$ is broken down per component below. The status LED figure is the one that dominates: all **four** status LEDs on the board (see [RGB LED](/LIA/docs/firmware/rgb-led/)) are wired in parallel per channel, so lighting a channel draws **4 × 0.15 A = 0.6 A**, not 0.15 A — driving one color drives all four LEDs at once.

| Component | Current | Notes |
| --- | --- | --- |
| ESP32-S3 (active) | ~50 mA | CPU + radio/GPS UART/SPI handling, Wi-Fi/BLE off |
| SAM-M10Q GPS (acquiring/tracking) | ~25–28 mA | Typical for this class of GNSS module; not yet independently measured on LIA hardware |
| SX1262 LoRa (RX idle + periodic TX average) | ~15 mA | TX briefly peaks far higher (100+ mA) at max power; this is a blended average over a send cycle |
| Status LED, one channel solid | ~600 mA | 4 LEDs × 150 mA, wired in parallel per channel (see above) |
| ESP32-S3 + peripherals, deep sleep | ~0.5 mA | PPC cuts SX1262/GPS power entirely; illustrative, not yet measured |

### Continuous ("Beacon", switch HIGH) mode

Never sleeps ($d = 1$), status LED solid the whole time:

$$
I_{awake} = 50 + 25 + 15 + 600 = 690\ \text{mA}
$$

| Battery | Capacity | Runtime (80% DoD) |
| --- | --- | --- |
| V1.0 (18650) | 3500 mAh | $2800 / 690 \approx 4.1$ hours |
| V1.1 (pouch cell) | 1000 mAh | $800 / 690 \approx 1.2$ hours |

The status LED alone is roughly 87% of the awake current in this mode — continuous/Beacon mode is meant for short, active sessions (a walk), not all-day use, and this is why.

### Sleep-cycle ("Tracker", switch LOW) mode

The status LED is off between wakes, so the awake current is lighter, but every wake is a real GPS reacquisition (PPC cuts GPS power entirely during sleep, so there's no hot-fix state to resume from). Each cycle is the awake time *plus* a fixed ~60-second sleep afterward (`kSleepCycleMs`, see [Sleep/wake tuning](#sleepwake-tuning) below) — it isn't a 60-second window containing the awake time. Using the one real cold-fix measurement so far — about 36 seconds, see [GPS](/LIA/docs/firmware/gps/) — the cycle is roughly $36 + 60 = 96$ seconds, giving a duty cycle $d \approx 36/96 \approx 0.375$:

$$
I_{awake} = 50 + 28 + 15 + 0 = 93\ \text{mA}, \quad I_{sleep} = 0.5\ \text{mA}
$$

$$
d \cdot I_{awake} + (1-d) \cdot I_{sleep} = 0.375 \times 93 + 0.625 \times 0.5 \approx 35.2\ \text{mA average}
$$

| Battery | Capacity | Runtime (80% DoD) |
| --- | --- | --- |
| V1.0 (18650) | 3500 mAh | $2800 / 35.2 \approx 79.5$ hours ($\approx$ 3.3 days) |
| V1.1 (pouch cell) | 1000 mAh | $800 / 35.2 \approx 22.7$ hours |

:::note
These numbers are illustrative, not measured current draw, and the 36s duty-cycle figure comes from a single cold-fix data point, not a validated average. Real GNSS fix time varies run to run, and self-discharge isn't modeled at all — see [Battery](/LIA/docs/firmware/battery/) for what's actually been measured on hardware so far, and [GPS](/LIA/docs/firmware/gps/) for measured fix times. Measured (not estimated) runtime numbers are still on the [roadmap](https://github.com/JGAguado/LIA#roadmap).
:::

:::caution
Sleep-cycle mode's real duty cycle is a strong function of GPS fix time, which this estimate treats as constant. A worse fix environment (indoors, dense canopy, urban canyon) that runs closer to the firmware's 90-second timeout on every wake stretches the cycle to $90 + 60 = 150$ seconds with $d = 90/150 = 0.6$ — average current climbs to $0.6 \times 93 + 0.4 \times 0.5 = 56$ mA, closer to **50 hours** (V1.0) / **14.3 hours** (V1.1) in that worst case, since more of each cycle is spent awake burning current without necessarily getting a fix.
:::

## Sleep/wake tuning

The main levers, in `firmware/services/TrackerService.h`:

- **Continuous-mode send interval** — 30 seconds, fixed.
- **Sleep-cycle interval** — roughly once a minute.
- **GPS fix timeout** — currently 90 seconds before a sleep-cycle wake gives up on a fix and sleeps anyway; not yet validated against real cold/warm-fix timing across a range of conditions.
- **Motion-triggered activity messages** — see [Accelerometer](/LIA/docs/firmware/accelerometer/); doesn't currently affect the sleep schedule itself.

## Peripheral power control

The `PPC` GPIO gates power to the SX1262 radio and the GNSS module together (they share a rail — there's no way to power one down without the other). The firmware sets this pin high before touching either peripheral and low before deep sleep, so both are fully powered down between wakes in sleep-cycle mode.

See also: [Battery](/LIA/docs/firmware/battery/), [Accelerometer](/LIA/docs/firmware/accelerometer/), [RGB LED](/LIA/docs/firmware/rgb-led/), and [Microcontroller](/LIA/docs/firmware/microcontroller/).
