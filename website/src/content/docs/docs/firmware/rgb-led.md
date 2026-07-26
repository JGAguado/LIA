---
title: RGB LED
description: Status indication with the on-board RED status LED.
---

LIA's status LED silkscreen footprint supports red, green, and blue channels, but on the current PCB revision, only the **RED** channel is actually usable — the green and blue channels are not wired for use and the firmware never touches them.

## Driving the LED

The RED channel is driven directly from the ESP32-S3 (`Lred`, GPIO40 — see [Microcontroller](/LIA/docs/firmware/microcontroller/)) via PWM, so brightness is adjustable, not just on/off. It's active-**high** — the channel lights when the pin is driven high, confirmed against real hardware (an earlier assumption that it was common-anode/active-low, matching a datasheet guess rather than the actual board, was corrected after a real-hardware test showed the LED behaving the opposite way of what the code intended).

## Status behaviour

- **Continuous (BMS switch HIGH) mode**: solid on by default.
- **Sleep-cycle (BMS switch LOW) mode**: off by default.
- **Manual override**: send `LED ON` / `LED OFF` on the private mesh channel (see [Meshtastic Configuration](/LIA/docs/firmware/meshtastic-configuration/)) to force it either way, overriding the automatic BMS-driven behaviour above for the rest of that boot. There's no "back to automatic" command — the next sleep-cycle wake (a full reboot) resets to automatic control.

See also: [Power Management](/LIA/docs/firmware/power-management/), [Meshtastic Configuration](/LIA/docs/firmware/meshtastic-configuration/), and [Microcontroller](/LIA/docs/firmware/microcontroller/).
