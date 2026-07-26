# Contributing to LIA

Thanks for considering a contribution. LIA spans hardware, firmware, mechanical design, and documentation, and contributions are welcome in all of them. This is an early-stage project, so expect things to move and change.

By participating, you're expected to follow the [Code of Conduct](CODE_OF_CONDUCT.md).

## Ways to contribute

- **Hardware** (`hardware/`) — schematic/PCB review, KiCad improvements, component sourcing feedback. Open the project in [KiCad](https://www.kicad.org/) 8+.
- **Enclosure** (`enclosure/`) — mechanical design feedback, print/manufacturing test reports, fit-check results. Native CAD files aren't tracked in git (see below); STEP/STL exports are.
- **Firmware** (`firmware/`) — Meshtastic configuration, power management tuning, flashing tools.
- **Website/Documentation** (`website/`) — an [Astro](https://astro.build/) + [Starlight](https://starlight.astro.build/) + [Tailwind CSS](https://tailwindcss.com/) static site.
- **Testing** — build a device, use it, and report back. Real-world battery life, range, and durability reports are especially valuable right now.

## Reporting bugs / requesting features

Open an issue using the appropriate [issue template](.github/ISSUE_TEMPLATE/). Include as much detail as you can — for hardware/firmware issues, that means the PCB/firmware revision and your region/radio settings; for website issues, that means the page URL and browser.

## Working on the website locally

```sh
cd website
npm install
npm run dev
```

Before opening a pull request, make sure these all pass:

```sh
npm run check        # astro type-check
npm run lint:md       # markdown lint
npm run format:check  # prettier check
npm run build          # production build
```

## A note on the enclosure's native CAD files

`enclosure/cad/` (native CAD source) is deliberately excluded from git via `.gitignore` — those files are large and require a commercial CAD license to open. If you need the native source to make an edit, open an issue and we'll coordinate sharing it directly.

## Pull requests

- Keep PRs focused on a single change.
- Describe *why* the change is needed, not just what changed.
- Link any related issue.
- Use the [pull request template](.github/PULL_REQUEST_TEMPLATE.md).
- CI must pass (build, markdown lint, link check, formatting) before merge.

## License

By contributing, you agree that your contributions will be licensed under the project's [CC BY-NC-SA 4.0 license](LICENSE).
