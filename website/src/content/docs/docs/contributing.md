---
title: Contributing
description: How to contribute to LIA.
---

LIA spans hardware, firmware, mechanical design, and this website — contributions are welcome across all of them. The full guide lives in [`CONTRIBUTING.md`](https://github.com/JGAguado/LIA/blob/main/CONTRIBUTING.md) at the root of the repository; this page is a quick summary.

## Ways to contribute

- **Hardware** — schematic/PCB review, KiCad improvements, sourcing feedback. See [PCB](/LIA/docs/hardware/pcb/) and [BOM](/LIA/docs/hardware/bom/).
- **Enclosure** — mechanical design feedback, print/manufacturing test reports. See [Enclosure](/LIA/docs/hardware/enclosure/).
- **Firmware** — Meshtastic configuration, power management tuning. See [Firmware](/LIA/docs/firmware/meshtastic-configuration/).
- **Documentation** — this site. It's an Astro + Starlight project under [`website/`](https://github.com/JGAguado/LIA/tree/main/website).
- **Testing** — build one, use it, and report back. Real-world battery life, range, and durability data is especially valuable while the project is this early.

## Working on the website locally

```sh
cd website
npm install
npm run dev
```

Before opening a pull request:

```sh
npm run check       # type-check
npm run lint:md      # markdown lint
npm run format:check # formatting
npm run build         # production build
```

## Pull requests

Keep PRs focused on one change, describe the *why* in the description, and link any related issue. See the [pull request template](https://github.com/JGAguado/LIA/blob/main/.github/PULL_REQUEST_TEMPLATE.md) for details.

See also: [License](/LIA/docs/license/).
