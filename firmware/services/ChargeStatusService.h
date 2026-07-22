#pragma once

#include "configuration.h"
#include "SinglePortModule.h"
#include "concurrency/OSThread.h"

/// Reports LiaBoard's charger status (TP4056 CHG/STBY) to the configured
/// target node as plain text, on the same private channel TrackerService
/// uses (MeshTargets.h) -- Phase 7, per explicit instruction (2026-07-22) to
/// send messages instead of the RED LED breathing/off behaviour
/// firmware/AGENTS.md originally specified for this phase.
///
/// Edge-triggered against LiaBoard::instance().isCharging() /
/// isChargeComplete(): sends "Charging" once when charging starts, "Device
/// charged" once when charge-complete is detected. Never repeats the same
/// message while its condition stays asserted.
///
/// Construct once from lateInitVariant(): `new ChargeStatusService();`.
/// Nothing needs to reference the instance afterwards -- MeshModule/OSThread
/// both self-register on construction -- so no global pointer is kept (see
/// firmware/AGENTS.md "No globals").
class ChargeStatusService : public SinglePortModule, private concurrency::OSThread
{
  public:
    ChargeStatusService();

  protected:
    int32_t runOnce() override;

  private:
    static constexpr uint32_t kPollIntervalMs = 3000;

    void sendText(const char *message);

    bool wasCharging_ = false;
    bool wasChargeComplete_ = false;
};
