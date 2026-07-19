#pragma once

#include "configuration.h"
#include "SinglePortModule.h"
#include "concurrency/OSThread.h"

/// Owns periodic mesh transmissions for LIA. Formalized Phase 2's ad-hoc
/// "LIA Radio Test" sender (which lived inline in
/// extra_variants/lia_v1/variant.cpp) into a proper service in Phase 3.5;
/// Phase 5 replaced the fixed text payload with the current GPS position
/// (meshtastic_Position, same as stock PositionModule sends), per
/// firmware/AGENTS.md's architecture.
///
/// The tracker/beacon state machine (Phase 6) is not implemented yet.
///
/// Construct once from lateInitVariant() (after the radio is initialized):
/// `new TrackerService();`. Nothing needs to reference the instance
/// afterwards -- MeshModule/OSThread both self-register on construction --
/// so no global pointer is kept (see firmware/AGENTS.md "No globals").
class TrackerService : public SinglePortModule, private concurrency::OSThread
{
  public:
    TrackerService();

  protected:
    int32_t runOnce() override;

  private:
    // Target node MAC E7:25:DC:E6:D6:63 -> NodeNum is the low 32 bits (see
    // NodeDB.cpp's nodeNum derivation). Single configuration constant per
    // firmware/AGENTS.md "Target Node" -- the one place this is defined.
    // Migrate to a configurable node ID in a later phase.
    static constexpr NodeNum kDestination = 0xDCE6D663;
};
