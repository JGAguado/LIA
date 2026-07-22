#pragma once

#include "configuration.h"
#include "SinglePortModule.h"
#include "concurrency/OSThread.h"
#include "lia/ImuMotionDriver.h"

#include <Adafruit_MAX1704X.h>

/// Responds to short text commands sent on the private kLiaChannelName
/// channel (see MeshTargets.h) -- lia_v1 only, since IMU_ON/IMU_OFF need the
/// physical LSM6DSOXTR IMU that lia_v2 removed. Replies go back as a DM to
/// whoever sent the command, on the channel it arrived on -- promiscuous, so
/// both a direct message and a broadcast on that channel work.
///
/// Supported commands (case-insensitive, see handleCommand()):
/// GPS, BATTERY, IMU_ON, IMU_OFF, CHG_ON, CHG_OFF, STB_ON, STB_OFF, HELP.
/// Unrecognized text is silently ignored, since this channel may carry
/// normal chat too, not just commands.
///
/// Construct once from lateInitVariant(), after ChargeStatusService (whose
/// instance() this reaches for CHG_ON/OFF and STB_ON/OFF): `new
/// CommandService();`. Nothing needs to reference this instance afterwards
/// -- MeshModule/OSThread both self-register on construction.
class CommandService : public SinglePortModule, private concurrency::OSThread
{
  public:
    CommandService();

  protected:
    ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) override;
    int32_t runOnce() override;

  private:
    // How often to poll the IMU driver for a pending interrupt while
    // IMU_ON is active. The interrupt itself is instant; this just bounds
    // how quickly we notice it (the ISR can't safely do the I2C read/send
    // itself -- see ImuMotionDriver).
    static constexpr uint32_t kImuActivePollMs = 250;
    // Idle cadence when IMU_ON hasn't been requested -- just thread upkeep.
    static constexpr uint32_t kIdlePollMs = 5000;

    void handleCommand(const meshtastic_MeshPacket &mp, const char *command);
    void sendText(NodeNum to, uint8_t channel, const char *text);
    // Reads the gauge directly rather than through Meshtastic's own
    // powerStatus->getBatteryChargePercent(): Power::setup() runs its
    // battery-source detection (including its own MAX17048 check) *before*
    // main.cpp's I2C scan populates the sensor map that check relies on, so
    // it always sees "not ready yet" at boot and falls back to reporting
    // 101% ("no battery, external power") -- confirmed on real hardware
    // (2026-07-22): the gauge is genuinely present, but that fallback value
    // never gets corrected later. Talking to the MAX17048 ourselves sidesteps
    // that ordering bug entirely.
    int batteryPercent();

    ImuMotionDriver imu_;
    bool imuActive_ = false;
    Adafruit_MAX17048 gauge_;
    bool gaugeBegun_ = false;
};
