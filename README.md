[![License: CC BY-NC-SA 4.0][license-shield]](LICENSE)
[![Project Maintenance][maintenance-shield]][maintenance]

<p align="center">
    <picture>
      <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/JGAguado/LIA/main/website/src/assets/brand/Primary%20dark.png">
      <source media="(prefers-color-scheme: light)" srcset="https://raw.githubusercontent.com/JGAguado/LIA/main/website/src/assets/brand/Primary%20light.png">
      <img alt="LIA logo" src="https://raw.githubusercontent.com/JGAguado/LIA/main/website/src/assets/brand/Primary%20light.png">
    </picture>
</p>


**LIA** is an open-source, low-power pet and asset tracker built on top of [Meshtastic](https://meshtastic.org/) and LoRa mesh networking. It's designed to be a compact, long-range, subscription-free tracking device that keeps working off-grid — no cell coverage, no monthly fee, no cloud dependency.

📖 **[Read the full documentation](https://jgaguado.github.io/LIA/)**

---

## Features

- Long-range LoRa communication over the open Meshtastic protocol
- GPS/GNSS location tracking, sent only as a direct message to a predefined target node — not visible to the wider public mesh
- A physical 3-position mode switch: continuous tracking (LED on, position every 30s), a low-power sleep cycle (wake roughly once a minute, get a fix, report it, sleep again), or fully OFF (cuts power to the MCU while still allowing the battery to charge)
- A text-command interface over the mesh: query position/battery on demand, toggle charge/motion notifications, override the status LED — see [Meshtastic Configuration](https://jgaguado.github.io/LIA/docs/firmware/meshtastic-configuration/)
- Deep-sleep, low-power firmware with real hardware wake sources (timer, mode-switch flip, charger plug-in)
- Subscription-free — no cloud backend required
- Fully open hardware: schematics, PCB layout, BOM, and enclosure CAD
- Compact, collar-mountable, waterproof-by-design enclosure — in three variants, see below

## Hardware Stack

**Core components**

- ESP32-S3-MINI-1 (MCU, Wi-Fi/BLE)
- SX1262 LoRa transceiver
- u-blox SAM-M10Q GNSS module
- MAX17048 battery fuel gauge

**Two hardware revisions, one firmware**

Both revisions carry the same reworked LSM6DSOXTR 6-axis IMU and run the identical `lia_v1` firmware — the only difference is the battery/enclosure:

- **V1.0** — 18650 Li-ion cell (3500 mAh) with magnetic pogo charging.
- **V1.1** — a 50×37×4 mm pouch cell (1000 mAh) instead, trading capacity for a much more compact enclosure.

**Antennas**

- Internal FPC LoRa antenna, or alternatively a standard omnidirectional linear 868/915 MHz antenna (e.g. [Seeed's LoRa Antenna Kit](https://www.seeedstudio.com/LoRa-Antenna-Kit-for-reTerminal-DM-p-5714.html))
- Ceramic GNSS patch antenna

See [Hardware documentation](https://jgaguado.github.io/LIA/docs/hardware/pcb/) for full details, and [Enclosure](https://jgaguado.github.io/LIA/docs/hardware/enclosure/) for the enclosure variants that pair with these boards.

## Software Stack

- [Meshtastic](https://meshtastic.org/) firmware
- [Astro](https://astro.build/) + [Starlight](https://starlight.astro.build/) + [Tailwind CSS](https://tailwindcss.com/) for this documentation site (fully static, no backend, no analytics, no cookies)

## Repository Structure

```
LIA/
├── website/       Astro + Starlight documentation & marketing site
├── firmware/       Meshtastic-based firmware (board variants, services, drivers, build tooling)
├── hardware/       KiCad project, PCB renders, schematics, BOM, production files
├── enclosure/       Enclosure CAD (gitignored native files), STEP/STL exports, renders — V1.0-Detachable/, V1.0-Fixed/, V1.1-Fixed/
├── mobile/         Companion mobile app (planned)
├── scripts/       Repo maintenance and tooling scripts
├── .github/       CI/CD workflows, issue & PR templates
├── LICENSE
├── CONTRIBUTING.md
├── CODE_OF_CONDUCT.md
└── README.md
```

## Quick Start

**Browse the docs:** https://jgaguado.github.io/LIA/

**Run the website locally:**

```sh
cd website
npm install
npm run dev
```

**Hardware:** open [`hardware/kicad/LIA.kicad_pro`](hardware/kicad/LIA.kicad_pro) in [KiCad](https://www.kicad.org/) 8+.

**Enclosure:** three versions — [`enclosure/V1.0-Detachable/`](enclosure/V1.0-Detachable), [`enclosure/V1.0-Fixed/`](enclosure/V1.0-Fixed), [`enclosure/V1.1-Fixed/`](enclosure/V1.1-Fixed) — each with its own `step/`/`stl/` exports and `drawings/`; no CAD license required to view or print them. See [Enclosure docs](https://jgaguado.github.io/LIA/docs/hardware/enclosure/) for what differs between them.

**Firmware:** see [`firmware/README.md`](firmware/README.md) and the [Getting Started guide](https://jgaguado.github.io/LIA/docs/getting-started/).

## Contributing

Contributions are welcome across hardware, firmware, mechanical design, and documentation. See [CONTRIBUTING.md](CONTRIBUTING.md) for how to get set up, and [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) for community guidelines.

## License

LIA is released under **[CC BY-NC-SA 4.0](LICENSE)** (Attribution-NonCommercial-ShareAlike). Personal, educational, and community use is welcome; commercial use requires permission from the maintainers. See the [License page](https://jgaguado.github.io/LIA/docs/license/) for details.

## Roadmap
- [x] Draft project requirements and design goals
- [x] Complete first hardware revision (V1.0) and a second, more compact revision (V1.1)
- [x] Complete first enclosure revisions — V1.0-Detachable, V1.0-Fixed, and V1.1-Fixed
- [x] Manufacture and assemble first batch of boards
- [x] Smoke-test and validate hardware (radio, GPS, charging, battery gauge, IMU)
- [x] Release first firmware — see [Development History](https://jgaguado.github.io/LIA/docs/development-history/) for the phase-by-phase log
- [x] Release initial documentation
- [ ] Field trial: real-world tracking validation over an extended period
- [ ] Measured (not estimated) battery runtime numbers

## Disclaimer

LIA is an experimental open-source project and should not be considered a guaranteed real-time safety tracking solution.

[license-shield]: https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg?style=for-the-badge
[maintenance-shield]: https://img.shields.io/badge/maintainer-J.%20G.%20Aguado-2e48a7.svg?style=for-the-badge
[maintenance]: https://github.com/JGAguado
