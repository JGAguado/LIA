#include "ChargeStatusService.h"

#include "ChannelLookup.h"
#include "MeshService.h"
#include "MeshTargets.h"
#include "lia/LiaBoard.h"

#include <cstring>

ChargeStatusService::ChargeStatusService()
    : SinglePortModule("ChargeStatus", meshtastic_PortNum_TEXT_MESSAGE_APP), concurrency::OSThread("ChargeStatus")
{
}

int32_t ChargeStatusService::runOnce()
{
    const bool charging = LiaBoard::instance().isCharging();
    const bool chargeComplete = LiaBoard::instance().isChargeComplete();

    if (charging && !wasCharging_)
        sendText("Charging");
    if (chargeComplete && !wasChargeComplete_)
        sendText("Device charged");

    wasCharging_ = charging;
    wasChargeComplete_ = chargeComplete;

    return kPollIntervalMs;
}

void ChargeStatusService::sendText(const char *message)
{
    int16_t channelIndex = findLiaChannelIndexByName(kLiaChannelName);
    if (channelIndex < 0) {
        // Fail closed, same reasoning as TrackerService: never fall back to
        // the default/public channel just because "Test" isn't configured.
        LOG_WARN("ChargeStatus: no channel named \"%s\" configured, skipping send (\"%s\")", kLiaChannelName, message);
        return;
    }

    meshtastic_MeshPacket *p = allocDataPacket();
    p->to = kLiaTargetNode;
    p->channel = (uint8_t)channelIndex;
    p->want_ack = false;
    p->decoded.want_response = false;

    size_t len = strlen(message);
    if (len > sizeof(p->decoded.payload.bytes))
        len = sizeof(p->decoded.payload.bytes);
    p->decoded.payload.size = len;
    memcpy(p->decoded.payload.bytes, message, len);

    service->sendToMesh(p);
    LOG_INFO("ChargeStatus: sent \"%s\" to 0x%08x on channel %d (\"%s\")", message, kLiaTargetNode, channelIndex,
              kLiaChannelName);
}
