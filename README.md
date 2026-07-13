[![License: CC BY-NC-SA 4.0][license-shield]](LICENSE)
[![Project Maintenance][maintenance-shield]][maintenance]

<p align="center">
    <picture>
      <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/JGAguado/LIA/main/website/src/assets/brand/Primary%20dark.png">
      <source media="(prefers-color-scheme: light)" srcset="https://raw.githubusercontent.com/JGAguado/LIA/main/website/src/assets/brand/Primary%20light.png">
      <img alt="LIA logo" src="https://raw.githubusercontent.com/JGAguado/LIA/main/website/src/assets/brand/Primary%20light.png">
    </picture>
</p>

<p align="center"><strong>Off-grid. On track.</strong></p>

**LIA** is an open-source, low-power pet and asset tracker built on top of [Meshtastic](https://meshtastic.org/) and LoRa mesh networking. It's designed to be a compact, long-range, subscription-free tracking device that keeps working off-grid — no cell coverage, no monthly fee, no cloud dependency.

📖 **[Read the full documentation](https://jgaguado.github.io/LIA/)**

---

## Features

- Long-range LoRa communication over the open Meshtastic protocol
- GPS/GNSS location tracking
- Deep-sleep, low-power firmware
- Subscription-free — no cloud backend required
- Fully open hardware: schematics, PCB layout, BOM, and enclosure CAD
- Compact, collar-mountable, waterproof-by-design enclosure

## Hardware Stack

**Core components**

- ESP32-S3-MINI-1 (MCU, Wi-Fi/BLE)
- SX1262 LoRa transceiver
- u-blox SAM-M10Q GNSS module
- 18650 Li-ion battery with magnetic pogo charging

**Antennas**

- Internal FPC LoRa antenna
- Ceramic GNSS patch antenna

**Optional sensors**

- Accelerometer for motion-triggered wake
- Environmental sensors

See [Hardware documentation](https://jgaguado.github.io/LIA/docs/hardware/pcb/) for full details.

## Software Stack

- [Meshtastic](https://meshtastic.org/) firmware
- [Astro](https://astro.build/) + [Starlight](https://starlight.astro.build/) + [Tailwind CSS](https://tailwindcss.com/) for this documentation site (fully static, no backend, no analytics, no cookies)

## Repository Structure

```
LIA/
├── website/       Astro + Starlight documentation & marketing site
├── firmware/       Meshtastic and custom firmware, flashing tools
├── hardware/       KiCad project, PCB renders, schematics, BOM, production files
├── enclosure/       Enclosure CAD (gitignored native files), STEP/STL exports, renders
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

**Enclosure:** STEP/STL exports are in [`enclosure/step/`](enclosure/step) and [`enclosure/stl/`](enclosure/stl) — no CAD license required to view or print them.

**Firmware:** see [`firmware/README.md`](firmware/README.md) and the [Getting Started guide](https://jgaguado.github.io/LIA/docs/getting-started/).

## Contributing

Contributions are welcome across hardware, firmware, mechanical design, and documentation. See [CONTRIBUTING.md](CONTRIBUTING.md) for how to get set up, and [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) for community guidelines.

## License

LIA is released under **[CC BY-NC-SA 4.0](LICENSE)** (Attribution-NonCommercial-ShareAlike). Personal, educational, and community use is welcome; commercial use requires permission from the maintainers. See the [License page](https://jgaguado.github.io/LIA/docs/license/) for details.

## Roadmap
- [x] Draft project requirements and design goals
- [x] Complete first hardware revision 
- [ ] Complete first enclosure revision
- [ ] Manufacture and assemble first batch of boards
- [ ] Smoke-test and validate hardware
- [ ] Release first firmware
- [ ] Release initial documentation

## Disclaimer

LIA is an experimental open-source project and should not be considered a guaranteed real-time safety tracking solution.

[license-shield]: https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg?style=for-the-badge
[maintenance-shield]: https://img.shields.io/badge/maintainer-J.%20G.%20Aguado-2e48a7.svg?style=for-the-badge
[maintenance]: https://github.com/JGAguado
