#include "ChargeStatusService.h"

#include "MeshService.h"
#include "MeshTargets.h"
#include "lia/LiaBoard.h"

#include <cstring>

ChargeStatusService *ChargeStatusService::instance_ = nullptr;

ChargeStatusService::ChargeStatusService()
    : SinglePortModule("ChargeStatus", meshtastic_PortNum_TEXT_MESSAGE_APP), concurrency::OSThread("ChargeStatus")
{
    instance_ = this;
}

int32_t ChargeStatusService::runOnce()
{
    const bool charging = LiaBoard::instance().isCharging();
    const bool chargeComplete = LiaBoard::instance().isChargeComplete();

    // Edge state is tracked regardless of whether notifications are
    // currently enabled, so re-enabling mid-condition doesn't immediately
    // re-fire for a transition that already happened.
    if (charging && !wasCharging_ && chargingNotificationsEnabled_)
        sendText("Charging");
    if (chargeComplete && !wasChargeComplete_ && chargeCompleteNotificationsEnabled_)
        sendText("Device charged");

    wasCharging_ = charging;
    wasChargeComplete_ = chargeComplete;

    return kPollIntervalMs;
}

void ChargeStatusService::sendText(const char *message)
{
    meshtastic_MeshPacket *p = allocDataPacket();
    p->to = kLiaTargetNode;
    p->want_ack = false;
    p->decoded.want_response = false;

    size_t len = strlen(message);
    if (len > sizeof(p->decoded.payload.bytes))
        len = sizeof(p->decoded.payload.bytes);
    p->decoded.payload.size = len;
    memcpy(p->decoded.payload.bytes, message, len);

    service->sendToMesh(p);
    LOG_INFO("ChargeStatus: sent \"%s\" to 0x%08x", message, kLiaTargetNode);
}
