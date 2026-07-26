#include "CommandService.h"

#include "ChargeStatusService.h"
#include "MeshService.h"
#include "MeshTargets.h"
#include "NodeDB.h"
#include "TrackerService.h"

#include <Adafruit_BusIO_Register.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_MAX1704X.h> // for the MAX17048_I2CADDR_DEFAULT / MAX1704X_SOC_REG constants only

#include <cctype>
#include <cstdio>
#include <cstring>

CommandService::CommandService()
    : SinglePortModule("Command", meshtastic_PortNum_TEXT_MESSAGE_APP), concurrency::OSThread("Command")
{
    isPromiscuous = true;
}

ProcessMessage CommandService::handleReceived(const meshtastic_MeshPacket &mp)
{
    // Only act on messages addressed directly to us -- per explicit
    // instruction (2026-07-26), superseding the earlier private-channel-name
    // design. A DM sent through the official app is commonly PKI-encrypted,
    // which bypasses the channel/PSK system entirely and reports channel=0
    // regardless of which channel was open at the time -- confirmed on real
    // hardware (2026-07-22).
    if (mp.to != nodeDB->getNodeNum())
        return ProcessMessage::CONTINUE;

    if (mp.decoded.payload.size == 0)
        return ProcessMessage::CONTINUE;

    char buf[64];
    size_t n = mp.decoded.payload.size;
    if (n > sizeof(buf) - 1)
        n = sizeof(buf) - 1;
    memcpy(buf, mp.decoded.payload.bytes, n);
    buf[n] = '\0';

    // Trim trailing whitespace/newline some clients append.
    while (n > 0 && std::isspace(static_cast<unsigned char>(buf[n - 1])))
        buf[--n] = '\0';

    // Trim leading whitespace and collapse internal whitespace runs to a
    // single space, so two-word commands ("LED ON") still match if someone
    // fat-fingers an extra space or a tab between the words.
    char *start = buf;
    while (*start && std::isspace(static_cast<unsigned char>(*start)))
        start++;
    char normalized[64];
    size_t ni = 0;
    bool lastWasSpace = false;
    for (char *c = start; *c && ni < sizeof(normalized) - 1; c++) {
        if (std::isspace(static_cast<unsigned char>(*c))) {
            if (!lastWasSpace)
                normalized[ni++] = ' ';
            lastWasSpace = true;
        } else {
            normalized[ni++] = *c;
            lastWasSpace = false;
        }
    }
    normalized[ni] = '\0';

    handleCommand(mp, normalized);
    return ProcessMessage::CONTINUE;
}

void CommandService::handleCommand(const meshtastic_MeshPacket &mp, const char *command)
{
    if (strcasecmp(command, "GPS") == 0) {
        bool haveFix = !(localPosition.latitude_i == 0 && localPosition.longitude_i == 0);
        if (haveFix) {
            char text[64];
            snprintf(text, sizeof(text), "Lat: %.6f, Lon: %.6f", localPosition.latitude_i * 1e-7,
                     localPosition.longitude_i * 1e-7);
            sendText(mp.from, text);
        } else {
            sendText(mp.from, "No GPS fix yet");
        }
    } else if (strcasecmp(command, "BATTERY") == 0) {
        int percent = batteryPercent();
        char text[32];
        if (percent >= 0)
            snprintf(text, sizeof(text), "Battery: %d%%", percent);
        else
            snprintf(text, sizeof(text), "Battery gauge not detected");
        sendText(mp.from, text);
    } else if (strcasecmp(command, "IMU ON") == 0) {
        if (imu_.begin()) {
            imuActive_ = true;
            sendText(mp.from, "IMU activity detection ON");
        } else {
            sendText(mp.from, "IMU not detected");
        }
    } else if (strcasecmp(command, "IMU OFF") == 0) {
        imu_.setInterruptEnabled(false);
        imuActive_ = false;
        sendText(mp.from, "IMU activity detection OFF");
    } else if (strcasecmp(command, "CHG ON") == 0) {
        if (ChargeStatusService::instance())
            ChargeStatusService::instance()->setChargingNotificationsEnabled(true);
        sendText(mp.from, "Charging notifications ON");
    } else if (strcasecmp(command, "CHG OFF") == 0) {
        if (ChargeStatusService::instance())
            ChargeStatusService::instance()->setChargingNotificationsEnabled(false);
        sendText(mp.from, "Charging notifications OFF");
    } else if (strcasecmp(command, "STB ON") == 0) {
        if (ChargeStatusService::instance())
            ChargeStatusService::instance()->setChargeCompleteNotificationsEnabled(true);
        sendText(mp.from, "Charge-complete notifications ON");
    } else if (strcasecmp(command, "STB OFF") == 0) {
        if (ChargeStatusService::instance())
            ChargeStatusService::instance()->setChargeCompleteNotificationsEnabled(false);
        sendText(mp.from, "Charge-complete notifications OFF");
    } else if (strcasecmp(command, "LED ON") == 0) {
        if (TrackerService::instance())
            TrackerService::instance()->setManualLed(true);
        sendText(mp.from, "LED ON");
    } else if (strcasecmp(command, "LED OFF") == 0) {
        if (TrackerService::instance())
            TrackerService::instance()->setManualLed(false);
        sendText(mp.from, "LED OFF");
    } else if (strcasecmp(command, "HELP") == 0) {
        sendText(mp.from,
                 "Commands: GPS, BATTERY, IMU ON, IMU OFF, CHG ON, CHG OFF, STB ON, STB OFF, LED ON, LED OFF, HELP");
    }
    // Unrecognized text is silently ignored -- a DM to this device may carry
    // normal chat too, not just commands.
}

int32_t CommandService::runOnce()
{
    if (imuActive_ && imu_.consumeActivityDetected()) {
        sendText(kLiaTargetNode, "Activity detected");
    }
    return imuActive_ ? kImuActivePollMs : kIdlePollMs;
}

int CommandService::batteryPercent()
{
    Adafruit_I2CDevice dev(MAX17048_I2CADDR_DEFAULT, &Wire);
    if (!dev.begin(false)) // false: skip the address-detect scan, already confirmed present at boot
        return -1;

    // SOC register: 16-bit value, 1/256ths of a percent -- same read and
    // scaling Adafruit_MAX17048::cellPercent() itself uses, just without
    // going through begin()'s reset. A plain read, no chip reconfiguration.
    Adafruit_BusIO_Register socReg(&dev, MAX1704X_SOC_REG, 2, MSBFIRST);
    uint32_t raw = socReg.read();

    float percent = raw / 256.0f;
    if (percent < 0)
        percent = 0;
    if (percent > 100)
        percent = 100;
    return static_cast<int>(percent + 0.5f);
}

void CommandService::sendText(NodeNum to, const char *text)
{
    meshtastic_MeshPacket *p = allocDataPacket();
    p->to = to;
    p->want_ack = false;
    p->decoded.want_response = false;

    size_t len = strlen(text);
    if (len > sizeof(p->decoded.payload.bytes))
        len = sizeof(p->decoded.payload.bytes);
    p->decoded.payload.size = len;
    memcpy(p->decoded.payload.bytes, text, len);

    service->sendToMesh(p);
    LOG_INFO("Command: sent \"%s\" to 0x%08x", text, to);
}
