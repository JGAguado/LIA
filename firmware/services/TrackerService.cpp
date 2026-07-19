#include "TrackerService.h"

#include "MeshService.h"
#include "airtime.h"

TrackerService::TrackerService() : SinglePortModule("Tracker", meshtastic_PortNum_TEXT_MESSAGE_APP), concurrency::OSThread("Tracker")
{
}

int32_t TrackerService::runOnce()
{
    // lateInitVariant() (main.cpp, right after initLoRa()) runs before
    // `airTime = new AirTime()` (main.cpp, ~40 lines later) -- router and
    // service are already constructed by lateInitVariant() time, but airTime
    // is not, and this thread's first runOnce() fires before setup() reaches
    // that line. Without this guard it dereferences a null AirTime* and
    // crashes (LoadProhibited) on every boot -- confirmed via addr2line.
    if (!airTime || !airTime->isTxAllowedChannelUtil(true))
        return 5000;

    const char *payload = "LIA Radio Test";
    meshtastic_MeshPacket *p = allocDataPacket();
    p->to = kDestination;
    p->want_ack = false;
    p->decoded.payload.size = strlen(payload);
    memcpy(p->decoded.payload.bytes, payload, p->decoded.payload.size);

    service->sendToMesh(p);
    LOG_INFO("TrackerService: sent \"%s\" to 0x%08x", payload, kDestination);

    return 30000;
}
