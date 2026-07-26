---
title: GPS
description: GNSS positioning on LIA.
---

LIA uses a [u-blox SAM-M10Q](https://www.u-blox.com/en/product/sam-m10q-module) GNSS module with a ceramic patch antenna, supporting GPS, Galileo, GLONASS, and BeiDou constellations concurrently.

## Fix behavior

- **Cold fix** (no recent almanac/ephemeris data) can take up to a minute or more under open sky — measured at roughly 36 seconds to first lock (10 satellites) on real hardware outdoors.
- **Warm/hot fix** (waking shortly after a previous fix) is typically much faster, since the module retains almanac data across LIA's short deep-sleep cycles.
- Fix time and accuracy both suffer under tree canopy or near buildings — expect degraded performance in dense woods, which is a common real-world case for a pet tracker.
- Indoors, expect no fix at all — the firmware correctly reports "no fix" rather than sending a stale or meaningless position (see [Power Management](/LIA/docs/firmware/power-management/) for how the sleep-cycle mode handles waiting for a fix before giving up on that cycle).

## Troubleshooting a missing fix

1. Confirm the GNSS antenna is connected and not obstructed by metal (including some batteries).
2. Test outdoors with a clear view of the sky before assuming a hardware fault.
3. Check [Troubleshooting](/LIA/docs/troubleshooting/) for more.

See also: [Power Management](/LIA/docs/firmware/power-management/) and [PCB](/LIA/docs/hardware/pcb/).
