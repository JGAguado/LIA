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
/// `new TrackerService();`. CommandService (lia_v1 only, constructed later)
/// reaches this one instance via instance() to act on LED_ON/LED_OFF -- a
/// self-registering static pointer set in the constructor, same pattern as
/// ChargeStatusService::instance() (see that class for why this isn't a
/// lazily-constructed Meyer's singleton like LiaBoard).
class TrackerService : public SinglePortModule, private concurrency::OSThread
{
  public:
    TrackerService();

    static TrackerService *instance() { return instance_; }

    /// Manual RED LED override, per explicit instruction (2026-07-24):
    /// once set, runOnce() stops driving the LED from BMS state on its own
    /// -- there is no "back to automatic" command, so this is a one-way
    /// switch for the lifetime of this boot (a fresh boot, e.g. after the
    /// BMS-LOW sleep cycle's reboot, starts back in automatic mode).
    void setManualLed(bool on);

  protected:
    int32_t runOnce() override;

  private:
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
    bool ledManualOverride_ = false;

    static TrackerService *instance_;
};
