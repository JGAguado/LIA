---
title: Manufacturing
description: How LIA boards are fabricated and assembled.
---

LIA's PCB is designed for turnkey fabrication and assembly through a standard PCBA service (with the idea of using [Seeed Studio Fusion service](https://www.seeedstudio.com/fusion.html)).

## Fabrication outputs

The files needed to order a fabrication/assembly run are generated from KiCad's fabrication toolkit and published under [`hardware/production/`](https://github.com/JGAguado/LIA/tree/main/hardware/production):

- Gerbers and drill files (`LIA.zip`)
- `bom.csv` — bill of materials
- `positions.csv` — pick-and-place coordinates
- `designators.csv` — designator/footprint reference

## Enclosure manufacturing

The enclosure is composed of parts designed for CNC machining and/or 3D printing (resin/FDM) from the STEP/STL exports under [`enclosure/step/`](https://github.com/JGAguado/LIA/tree/main/enclosure/step) and [`enclosure/stl/`](https://github.com/JGAguado/LIA/tree/main/enclosure/stl). No production-scale manufacturing process (injection molding, tooling) has been defined yet — this is still a prototype-stage design.

:::note
If you fabricate a batch of boards or enclosures and want to document your process (fab house, settings, yield), please contribute it back — see [Contributing](/LIA/docs/contributing/).
:::

See also: [BOM](/LIA/docs/hardware/bom/) and [Downloads](/LIA/docs/downloads/).
