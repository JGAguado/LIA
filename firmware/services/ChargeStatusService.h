#pragma once

#include "configuration.h"
#include "SinglePortModule.h"
#include "concurrency/OSThread.h"

/// Reports LiaBoard's charger status (TP4056 CHG/STBY) to the configured
/// target node as a direct message (MeshTargets.h's kLiaTargetNode) -- Phase
/// 7, per explicit instruction (2026-07-22) to send messages instead of the
/// RED LED breathing/off behaviour firmware/AGENTS.md originally specified
/// for this phase.
///
/// Edge-triggered against LiaBoard::instance().isCharging() /
/// isChargeComplete(): sends "Charging" once when charging starts, "Device
/// charged" once when charge-complete is detected. Never repeats the same
/// message while its condition stays asserted.
///
/// Construct once from lateInitVariant(): `new ChargeStatusService();`.
/// CommandService (constructed afterwards) reaches this one instance via
/// instance() to act on CHG ON/CHG OFF/STB ON/STB OFF commands -- a
/// self-registering static pointer set in the constructor, the same pattern
/// Meshtastic's own globals (nodeDB, service, screen, ...) use, rather than
/// a lazily-constructed Meyer's singleton like LiaBoard: this object's
/// construction time matters (it must happen from lateInitVariant(), for
/// the same MeshModule/OSThread self-registration ordering reasons as
/// TrackerService), so it can't be created on first access.
class ChargeStatusService : public SinglePortModule, private concurrency::OSThread
{
  public:
    ChargeStatusService();

    static ChargeStatusService *instance() { return instance_; }

    void setChargingNotificationsEnabled(bool enabled) { chargingNotificationsEnabled_ = enabled; }
    void setChargeCompleteNotificationsEnabled(bool enabled) { chargeCompleteNotificationsEnabled_ = enabled; }

  protected:
    int32_t runOnce() override;

  private:
    static constexpr uint32_t kPollIntervalMs = 3000;

    void sendText(const char *message);

    bool wasCharging_ = false;
    bool wasChargeComplete_ = false;
    bool chargingNotificationsEnabled_ = true;
    bool chargeCompleteNotificationsEnabled_ = true;

    static ChargeStatusService *instance_;
};
