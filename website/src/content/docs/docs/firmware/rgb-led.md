---
title: RGB LED
description: Status indication with LIA's RGB status LED.
---

LIA's status indicator is a full RGB LED, in four places on the board wired in parallel per channel (see [Microcontroller](/LIA/docs/firmware/microcontroller/) for the pinout) — driving a channel lights all four at once, for visibility from any angle once the tracker is mounted on a collar. See [Development History](/LIA/docs/development-history/) for a manufacturing note that currently limits the assembled units to the RED channel only.

## Driving the LED

The RED channel is driven directly from the ESP32-S3 (`Lred`, GPIO40) via PWM, so brightness is adjustable, not just on/off. It's active-**high** — the channel lights when the pin is driven high, confirmed against real hardware (an earlier assumption that it was common-anode/active-low, matching a datasheet guess rather than the actual board, was corrected after a real-hardware test showed the LED behaving the opposite way of what the code intended). The GREEN (`Lgreen`, GPIO41) and BLUE (`Lblue`, GPIO42) channels share the same wiring pattern.

## Status behaviour

- **Continuous (BMS switch HIGH) mode**: solid RED by default.
- **Sleep-cycle (BMS switch LOW) mode**: off by default.
- **Manual override**: send `LED ON` / `LED OFF` as a direct message to the device (see [Meshtastic Configuration](/LIA/docs/firmware/meshtastic-configuration/)) to force it either way, overriding the automatic BMS-driven behaviour above for the rest of that boot. There's no "back to automatic" command — the next sleep-cycle wake (a full reboot) resets to automatic control.

See also: [Power Management](/LIA/docs/firmware/power-management/), [Meshtastic Configuration](/LIA/docs/firmware/meshtastic-configuration/), and [Microcontroller](/LIA/docs/firmware/microcontroller/).
