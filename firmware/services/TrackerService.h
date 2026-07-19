#pragma once

#include "configuration.h"
#include "SinglePortModule.h"
#include "concurrency/OSThread.h"

/// Owns periodic mesh transmissions and the tracker/beacon state machine for
/// LIA, per firmware/AGENTS.md's architecture. Formalized Phase 2's ad-hoc
/// "LIA Radio Test" sender (which lived inline in
/// extra_variants/lia_v1/variant.cpp) into a proper service in Phase 3.5;
/// Phase 5 replaced the fixed text payload with the current GPS position
/// (meshtastic_Position, same as stock PositionModule sends); Phase 6 added
/// the BMS-driven behaviour switch.
///
/// BMS HIGH ("continuous", per AGENTS.md Phase 6 + Final Success Criteria):
/// RED LED solid on, send on a fixed cadence, never sleep.
/// BMS LOW ("sleep-cycle"): send once, then deep sleep for a minute (PPC is
/// cut for the duration via LiaBoard's notifyDeepSleep hook) and let the next
/// wake's fresh boot repeat the cycle. AGENTS.md and MOD.md disagreed on
/// which switch position gets which behaviour; resolved in favour of
/// AGENTS.md (2026-07-19) -- MOD.md's polarity is not used.
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

    // BMS HIGH: keep this send cadence indefinitely, no sleep.
    static constexpr uint32_t kContinuousIntervalMs = 30000;
    // BMS LOW: deep sleep this long between wake/send cycles ("wake every minute").
    static constexpr uint32_t kSleepCycleMs = 60000;
    // BMS LOW: how long to stay awake hunting for a GPS fix before giving up
    // on this cycle (send whatever's cached, or skip) and sleeping anyway --
    // an open, real-world-tunable tradeoff (see website's power-management.md
    // "GPS fix timeout" note), not a value validated against actual cold-fix
    // timing yet.
    static constexpr uint32_t kFixWaitTimeoutMs = 90000;

    void sendPosition();

    const uint32_t bootMs_ = millis();
    bool sleepOnNextExecution_ = false;
};
