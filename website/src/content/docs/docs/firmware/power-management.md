---
title: Power Management
description: Deep sleep, wake cycles, the BMS switch, and estimating battery life.
---

LIA's firmware spends most of its time in deep sleep when tracking continuously isn't needed, waking only to acquire a GPS fix and broadcast it over the mesh. The tradeoff is straightforward: shorter sleep intervals mean fresher position data but shorter battery life.

## The BMS switch

The board has an accessible physical switch (`BMS` GPIO) with two positions:

- **HIGH — continuous ("Beacon") mode.** The RED status LED stays solid on, the device never sleeps, and it broadcasts its position every 30 seconds. Intended for short-term, active use (a walk, actively searching for the animal) — it drains the battery quickly compared to the alternative below.
- **LOW — sleep-cycle ("Tracker") mode.** The device wakes roughly once a minute, tries to get a fresh GPS fix (giving up after 90 seconds and sleeping anyway if none arrives, rather than draining the battery hunting indefinitely), sends a position if it got one, then cuts power to the radio and GPS and goes back into deep sleep. This is the low-power default for everyday wear.

There is no third "power off" position — powering the device fully down means disconnecting the battery.

Toggling the switch takes effect on the next wake without needing a reflash. Two additional wake sources besides the once-a-minute timer let the device react to the switch (or a charger) sooner rather than waiting out the full interval:

- Flipping the switch to HIGH while the device is mid-sleep wakes it immediately.
- Connecting a charger while the device is mid-sleep also wakes it immediately (there's no separate USB-presence pin on the board, so the charger-detect signal is used as the closest available proxy for "USB connected").

## RED LED

- In continuous mode, the RED LED is on by default, driven automatically from the switch state.
- Sending **`LED ON`** / **`LED OFF`** on the private channel overrides this manually for the rest of that boot — see [RGB LED](/LIA/docs/firmware/rgb-led/) and [Meshtastic Configuration](/LIA/docs/firmware/meshtastic-configuration/). There's no "back to automatic" command; a fresh boot (e.g. the next sleep-cycle wake) resets to automatic control.

## Estimating battery life

A rough estimate for runtime $T$ (in hours) given a battery capacity $C$ (mAh), average current draw while awake $I_{awake}$ (mA), average sleep current $I_{sleep}$ (mA), duty cycle $d$ (fraction of time awake), and depth-of-discharge margin $m$:

$$
T = \frac{C \cdot m}{d \cdot I_{awake} + (1 - d) \cdot I_{sleep}}
$$

For example, with a 3500 mAh cell (V1's 18650, see [Battery](/LIA/docs/firmware/battery/)), an 80% usable depth of discharge, a duty cycle of 1% (waking for a fix and a broadcast every minute in sleep-cycle mode), 120 mA average draw while awake (radio + GPS fix), and 0.5 mA deep-sleep current:

$$
T = \frac{3500 \times 0.8}{0.01 \times 120 + 0.99 \times 0.5} \approx 1652\ \text{hours} \approx 69\ \text{days}
$$

> [!NOTE]
> These numbers are illustrative, not measured current draw. Real GNSS cold-fix time, radio duty cycle, and self-discharge will move this estimate — see [Battery](/LIA/docs/firmware/battery/) for what's actually been measured on hardware so far, and [GPS](/LIA/docs/firmware/gps/) for measured fix times.

## Sleep/wake tuning

The main levers, in `firmware/services/TrackerService.h`:

- **Continuous-mode send interval** — 30 seconds, fixed.
- **Sleep-cycle interval** — roughly once a minute.
- **GPS fix timeout** — currently 90 seconds before a sleep-cycle wake gives up on a fix and sleeps anyway; not yet validated against real cold/warm-fix timing across a range of conditions.
- **Motion-triggered activity messages** — see [Accelerometer](/LIA/docs/firmware/accelerometer/); doesn't currently affect the sleep schedule itself.

## Peripheral power control

The `PPC` GPIO gates power to the SX1262 radio and the GNSS module together (they share a rail — there's no way to power one down without the other). The firmware sets this pin high before touching either peripheral and low before deep sleep, so both are fully powered down between wakes in sleep-cycle mode.

See also: [Battery](/LIA/docs/firmware/battery/), [Accelerometer](/LIA/docs/firmware/accelerometer/), [RGB LED](/LIA/docs/firmware/rgb-led/), and [Microcontroller](/LIA/docs/firmware/microcontroller/).
