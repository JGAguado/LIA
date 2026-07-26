#include "TrackerService.h"

#include "MeshService.h"
#include "MeshTargets.h"
#include "NodeDB.h"
#include "airtime.h"
#include "lia/LiaBoard.h"
#include "sleep.h"

TrackerService *TrackerService::instance_ = nullptr;

TrackerService::TrackerService() : SinglePortModule("Tracker", meshtastic_PortNum_POSITION_APP), concurrency::OSThread("Tracker")
{
    instance_ = this;
}

void TrackerService::setManualLed(bool on)
{
    ledManualOverride_ = true;
    LiaBoard::instance().setRedLed(on ? 255 : 0);
}

int32_t TrackerService::runOnce()
{
    const bool continuous = LiaBoard::instance().isBmsHigh();
    if (!ledManualOverride_)
        LiaBoard::instance().setRedLed(continuous ? 255 : 0);

    // Scheduled last cycle, after sending: the packet has had a few seconds
    // to actually get on air, so it's safe to cut power and sleep now. This
    // mirrors stock PositionModule.cpp's own sleepOnNextExecution pattern.
    if (sleepOnNextExecution_) {
        sleepOnNextExecution_ = false;
        LOG_INFO("TrackerService: entering deep sleep for %us (BMS low)", kSleepCycleMs / 1000);
        doDeepSleep(kSleepCycleMs, false, false);
        return kSleepCycleMs; // unreachable -- doDeepSleep() does not return
    }

    if (!airTime || !airTime->isTxAllowedChannelUtil(true))
        return 5000;

    bool haveFix = !(localPosition.latitude_i == 0 && localPosition.longitude_i == 0);
    if (!haveFix) {
        if (continuous || (millis() - bootMs_ < kFixWaitTimeoutMs)) {
            LOG_WARN("TrackerService: no GPS fix yet");
            return continuous ? kContinuousIntervalMs : 5000;
        }
        // BMS low and out of patience for this cycle -- go back to sleep
        // rather than hunt for a fix indefinitely and drain the battery.
        LOG_WARN("TrackerService: no fix after %us, sleeping anyway", kFixWaitTimeoutMs / 1000);
        sleepOnNextExecution_ = true;
        return 5000;
    }

    sendPosition();

    if (!continuous) {
        sleepOnNextExecution_ = true;
        return 5000;
    }
    return kContinuousIntervalMs;
}

void TrackerService::sendPosition()
{
    meshtastic_Position pos = meshtastic_Position_init_default;
    pos.latitude_i = localPosition.latitude_i;
    pos.longitude_i = localPosition.longitude_i;
    pos.has_latitude_i = true;
    pos.has_longitude_i = true;
    pos.altitude = localPosition.altitude;
    pos.has_altitude = true;
    pos.time = localPosition.time;

    meshtastic_MeshPacket *p = allocDataPacket();
    p->to = kLiaTargetNode;
    p->want_ack = false;
    p->decoded.payload.size = pb_encode_to_bytes(p->decoded.payload.bytes, sizeof(p->decoded.payload.bytes),
                                                  &meshtastic_Position_msg, &pos);

    service->sendToMesh(p);
    LOG_INFO("TrackerService: sent position (lat=%d, lon=%d) to 0x%08x", pos.latitude_i, pos.longitude_i, kLiaTargetNode);
}
