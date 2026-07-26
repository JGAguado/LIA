---
title: Assembly
description: Putting an assembled PCB into the LIA enclosure.
---

> [!CAUTION]
> A validated, step-by-step assembly procedure hasn't shipped yet — LIA is still on its first hardware revision. This page describes the intended process; treat it as a draft until it's been through a full build.

## Prerequisites

- An assembled LIA PCB (see [PCB](/LIA/docs/hardware/pcb/) and [BOM](/LIA/docs/hardware/bom/))
- A printed or manufactured enclosure (see [Enclosure](/LIA/docs/hardware/enclosure/))
- An 18650 Li-ion cell
- 2x M2x3 screws for the board enclosure
- 4x M3x4 screws for the collar mechanism

## Tracking unit assembly

1. Flash and smoke-test the board **before** enclosing it — see [Getting Started](/LIA/docs/getting-started/).
2. Seat the 18650 cell in the battery clip, observing polarity.
3. Route the LoRa antenna, already clipped on the SX1262, along the inside of the top shell.
4. Lower the PCB into the bottom shell onto its standoffs.
5. Fit the top shell and side windows, then close and screw top and bottom shells of the tracker enclosure.
6. Confirm the status LEDs are visible through the window and the board still gets a GPS/mesh fix once sealed.

See also: [Manufacturing](/LIA/docs/manufacturing/).
