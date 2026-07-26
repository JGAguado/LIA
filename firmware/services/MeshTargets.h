#pragma once

#include "configuration.h"

/// Shared mesh-destination configuration for LIA's services, per
/// firmware/AGENTS.md "Target Node" -- defined once here so TrackerService,
/// ChargeStatusService, and CommandService agree.
///
/// Everything LIA-specific (position broadcasts, charge-status messages,
/// CommandService replies/pushes) is a direct message to this single node,
/// not a channel broadcast -- per explicit instruction (2026-07-26),
/// superseding the earlier private-channel-name design (see git history /
/// firmware/docs/phase-status.md for that design's rationale and why it was
/// dropped in favour of this simpler one).

// MAC E7:25:DC:E6:D6:63 -> NodeNum is the low 32 bits (see NodeDB.cpp's
// nodeNum derivation). Migrate to a configurable node ID in a later phase.
constexpr NodeNum kLiaTargetNode = 0xDCE6D663;
