---
title: Assembly
description: Putting an assembled PCB into the LIA enclosure.
---

## Prerequisites

- An assembled LIA PCB (see [PCB](/LIA/docs/hardware/pcb/) and [BOM](/LIA/docs/hardware/bom/))
- A printed or manufactured enclosure (see [Enclosure](/LIA/docs/hardware/enclosure/))
- An 18650 Li-ion cell
- External LoRa antenna (FPC LoRa antenna or alternative omnidirectional antenna, and GNSS patch antenna) — see [PCB](/LIA/docs/hardware/pcb/#antennas)

## Tracking unit assembly

1. Seat the 18650 cell in the battery clip, observing polarity and clip the antenna to the SX1262 module.
2. Flash and smoke-test the board **before** enclosing it — see [Getting Started](/LIA/docs/getting-started/).
3. Route the LoRa antenna, along the inside of the top shell (the stock FPC antenna or the alternative omnidirectional antenna — see [PCB](/LIA/docs/hardware/pcb/#antennas) — both use the same connector).
4. Lower the PCB into the bottom shell onto its standoffs.
5. Fit the top shell and side windows, then close and screw top and bottom shells of the tracker enclosure.
6. Confirm the status LEDs are visible through the window and the board still gets a GPS/mesh fix once sealed.

See also: [Manufacturing](/LIA/docs/manufacturing/).
