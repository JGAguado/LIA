# Services

## TrackerService

Owns periodic mesh transmissions. Sends the current GPS position
(`meshtastic_Position`, port `POSITION_APP`) as a unicast every 30s, built
from the global `localPosition` (`NodeDB.h`) the same way stock
`PositionModule.cpp` does -- skips the send if there's no fix yet (lat/lon
both zero) rather than sending a meaningless (0,0). The destination NodeNum
is the single `kDestination` constant in `TrackerService.h`, per
`firmware/AGENTS.md` "Target Node".

Phase history: Phase 2 introduced it inline in
`extra_variants/lia_v1/variant.cpp` as a fixed "LIA Radio Test" text payload;
Phase 3.5 moved it here as a proper `SinglePortModule` + `OSThread`; Phase 5
replaced the text payload with the real position.

Not yet implemented:

- The tracker/beacon state machine (Phase 6) -- gated on resolving the BMS
  switch polarity question, see `../board/README.md#open-questions`.

Constructed once from `lateInitVariant()` (`new TrackerService();`) --
nothing else needs to reference the instance, so no global pointer is kept
(MeshModule/OSThread both self-register on construction).
