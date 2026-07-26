---
title: Troubleshooting
description: Common problems and how to diagnose them.
---

## Device won't power on

- Confirm the 18650 cell is seated with correct polarity.
- Check the cell's voltage independently — over-discharged protected cells can latch off until charged.
- Try charging over the magnetic pogo connector for a few minutes before assuming a hardware fault.

## Device flashes but never boots past the first second or two

Seen on the `lia_v1` custom build (not stock Meshtastic) on the first
assembled board: serial output stops right after CPU frequency is set, before
LoRa/GPS initialize. See
[Building the Firmware — Known issues](/LIA/docs/firmware/building-the-firmware/#known-issues-on-first-hardware)
for what's been ruled out so far (not Bluetooth-related) and the leading
suspect (an I2C bus lockup during Meshtastic's sensor scan — check `SDA`/`SCL`
pull-ups and whether the IMU/fuel gauge are actually populated on your board).

## No GPS fix

- See [GPS](/LIA/docs/firmware/gps/) — most "no fix" reports are actually a cold fix taking longer than expected, or testing indoors.
- Confirm the GNSS antenna connector is fully seated.

## Not visible on the mesh

- Confirm the [region](/LIA/docs/firmware/lora/) is set — an unset region silently disables the radio.
- Check that at least one other Meshtastic node is within range and on the same channel/region.
- Confirm the node role is set to `TRACKER` — see [Meshtastic Configuration](/LIA/docs/firmware/meshtastic-configuration/).

## Battery drains faster than expected

- Review the broadcast interval and duty cycle in [Power Management](/LIA/docs/firmware/power-management/) — frequent position broadcasts cost real battery life.
- Confirm deep sleep is actually being entered (some debug/logging builds keep peripherals awake).

:::tip
If none of this helps, open an issue on [GitHub](https://github.com/JGAguado/LIA/issues) with your firmware version, region setting, and a description of what you're seeing.
:::

See also: [FAQ](/LIA/docs/faq/).
