#include "TrackerService.h"

#include "MeshService.h"
#include "NodeDB.h"
#include "airtime.h"

TrackerService::TrackerService() : SinglePortModule("Tracker", meshtastic_PortNum_POSITION_APP), concurrency::OSThread("Tracker")
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

    // Same skip stock PositionModule uses: no fix yet means lat/lon are both
    // still zero, and a (0,0) position is meaningless -- try again next cycle
    // instead of sending it.
    if (localPosition.latitude_i == 0 && localPosition.longitude_i == 0) {
        LOG_WARN("TrackerService: no GPS fix yet, skipping send");
        return 30000;
    }

    meshtastic_Position pos = meshtastic_Position_init_default;
    pos.latitude_i = localPosition.latitude_i;
    pos.longitude_i = localPosition.longitude_i;
    pos.has_latitude_i = true;
    pos.has_longitude_i = true;
    pos.altitude = localPosition.altitude;
    pos.has_altitude = true;
    pos.time = localPosition.time;

    meshtastic_MeshPacket *p = allocDataPacket();
    p->to = kDestination;
    p->want_ack = false;
    p->decoded.payload.size = pb_encode_to_bytes(p->decoded.payload.bytes, sizeof(p->decoded.payload.bytes),
                                                  &meshtastic_Position_msg, &pos);

    service->sendToMesh(p);
    LOG_INFO("TrackerService: sent position (lat=%d, lon=%d) to 0x%08x", pos.latitude_i, pos.longitude_i, kDestination);

    return 30000;
}
