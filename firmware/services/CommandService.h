#pragma once

#include "configuration.h"
#include "SinglePortModule.h"
#include "concurrency/OSThread.h"
#include "lia/ImuMotionDriver.h"

/// Responds to short text commands sent on the private kLiaChannelName
/// channel (see MeshTargets.h) -- lia_v1 only, since "IMU ON"/"IMU OFF" need
/// the physical LSM6DSOXTR IMU that lia_v2 removed. Replies go back as a DM
/// to whoever sent the command, on the channel it arrived on -- promiscuous,
/// so both a direct message and a broadcast on that channel work.
///
/// Supported commands (case-insensitive, whitespace-normalized -- see
/// handleReceived() -- so "led  off" / "Led Off" all match "LED OFF", see
/// handleCommand()): GPS, BATTERY, IMU ON, IMU OFF, CHG ON, CHG OFF,
/// STB ON, STB OFF, LED ON, LED OFF, HELP.
/// Unrecognized text is silently ignored, since this channel may carry
/// normal chat too, not just commands.
///
/// Construct once from lateInitVariant(), after TrackerService (whose
/// instance() this reaches for LED ON/OFF) and ChargeStatusService (whose
/// instance() this reaches for CHG ON/OFF and STB ON/OFF): `new
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
    // "IMU ON" is active. The interrupt itself is instant; this just bounds
    // how quickly we notice it (the ISR can't safely do the I2C read/send
    // itself -- see ImuMotionDriver).
    static constexpr uint32_t kImuActivePollMs = 250;
    // Idle cadence when "IMU ON" hasn't been requested -- just thread upkeep.
    static constexpr uint32_t kIdlePollMs = 5000;

    void handleCommand(const meshtastic_MeshPacket &mp, const char *command);
    void sendText(NodeNum to, uint8_t channel, const char *text);
    // Reads the gauge's SOC register directly over I2C -- deliberately not
    // via Adafruit_MAX17048::begin(), which sends the chip a hardware reset
    // command (0x5400) before every read; reading cellPercent() right after
    // catches the fuel-gauge algorithm before it has reconverged, giving a
    // near-zero reading every time (confirmed on real hardware, 2026-07-24).
    // Also not via Meshtastic's own powerStatus->getBatteryChargePercent():
    // Power::setup() runs its own MAX17048 detection *before* main.cpp's I2C
    // scan populates the sensor map that detection depends on, so it always
    // sees "not ready yet" at boot and locks in a 101% ("no battery")
    // fallback that never gets corrected later (confirmed 2026-07-22).
    // Returns -1 if the gauge doesn't respond.
    int batteryPercent();

    ImuMotionDriver imu_;
    bool imuActive_ = false;
};
