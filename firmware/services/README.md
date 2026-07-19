# Services

## TrackerService

Owns periodic mesh transmissions. Currently (Phase 3.5) still just the fixed
"LIA Radio Test" unicast payload from Phase 2, now formalized as a proper
`SinglePortModule` + `OSThread` instead of living inline in
`extra_variants/lia_v1/variant.cpp`. The destination NodeNum is the single
`kDestination` constant in `TrackerService.h`, per `firmware/AGENTS.md`
"Target Node".

Not yet implemented, planned for later phases:

- GPS transmission scheduling (Phase 5) -- replace the fixed text payload
  with the current GPS position.
- The tracker/beacon state machine (Phase 6) -- gated on resolving the BMS
  switch polarity question, see `../board/README.md#open-questions`.

Constructed once from `lateInitVariant()` (`new TrackerService();`) --
nothing else needs to reference the instance, so no global pointer is kept
(MeshModule/OSThread both self-register on construction).
