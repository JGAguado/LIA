// Standalone MAX17048 battery gauge bring-up test for LIA v1.
//
// No Meshtastic -- isolates a real-hardware bug where CommandService's
// BATTERY command always reports ~0% regardless of actual charge (see
// firmware/docs/phase-status.md). Prints two independently-computed
// readings side by side every 2s:
//
//   [raw]     A direct SOC/VCELL register read via Adafruit_I2CDevice +
//             Adafruit_BusIO_Register, deliberately never calling
//             Adafruit_MAX17048::begin() (which sends the chip a hardware
//             reset command, 0x5400, before every use) -- this is the same
//             approach CommandService::batteryPercent() uses.
//   [library] The stock Adafruit_MAX17048 class, begin()'d once at setup()
//             (so its one-time reset happens before any readings are
//             taken and logged here, same as it would in Meshtastic).
//
// If both stay stuck near 0% indefinitely, the bug isn't the reset-timing
// theory (that would show [library] converging upward over the first
// samples while [raw] stayed correct throughout) -- something else is
// wrong (wiring, address, chip state, scaling). Chip ID/IC version are
// also printed once at boot to confirm the gauge actually responds at all.
//
// Pin numbers match firmware/AGENTS.md's pinout (verified against
// hardware/kicad/LIA.kicad_sch) -- kept independent of firmware/meshtastic/
// on purpose, so this has no dependency on a Meshtastic checkout.

#include <Arduino.h>
#include <Wire.h>

#include <Adafruit_BusIO_Register.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_MAX1704X.h>

namespace
{
constexpr int kPinPPC = 21;
constexpr int kSdaPin = 4;
constexpr int kSclPin = 5;
constexpr uint32_t kReportIntervalMs = 2000;

Adafruit_MAX17048 gauge;
bool gaugeBegun = false;
uint32_t lastReportMs = 0;

// Same technique as CommandService::batteryPercent() -- a plain register
// read, no reset/reconfiguration of the chip.
bool readRaw(float &socPercent, float &cellVoltage)
{
    Adafruit_I2CDevice dev(MAX17048_I2CADDR_DEFAULT, &Wire);
    if (!dev.begin(false))
        return false;

    Adafruit_BusIO_Register socReg(&dev, MAX1704X_SOC_REG, 2, MSBFIRST);
    Adafruit_BusIO_Register vcellReg(&dev, MAX1704X_VCELL_REG, 2, MSBFIRST);

    socPercent = socReg.read() / 256.0f;
    cellVoltage = vcellReg.read() * 78.125f / 1000000.0f; // 78.125uV/LSB per datasheet

    return true;
}
} // namespace

void setup()
{
    Serial.begin(115200);
    delay(1500); // let the native-USB CDC host enumerate before we print anything

    Serial.println("LIA battery gauge test -- no Meshtastic, raw vs. library reads every 2s");

    pinMode(kPinPPC, OUTPUT);
    digitalWrite(kPinPPC, HIGH); // in case the gauge is on the PPC-gated rail
    delay(100);

    Wire.begin(kSdaPin, kSclPin);
    Serial.printf("I2C started on SDA=%d SCL=%d\n", kSdaPin, kSclPin);

    gaugeBegun = gauge.begin(&Wire); // this call resets the chip (0x5400) -- once, here
    Serial.printf("Adafruit_MAX17048::begin() -> %s\n", gaugeBegun ? "OK" : "FAILED");
    if (gaugeBegun) {
        Serial.printf("Chip ID: 0x%02X, IC version: 0x%04X\n", gauge.getChipID(), gauge.getICversion());
    }
}

void loop()
{
    uint32_t now = millis();
    if (now - lastReportMs < kReportIntervalMs)
        return;
    lastReportMs = now;

    float rawSoc = NAN, rawVCell = NAN;
    bool rawOk = readRaw(rawSoc, rawVCell);

    Serial.printf("t=%lus ", now / 1000);
    if (rawOk)
        Serial.printf("[raw] SOC=%.2f%% VCELL=%.4fV  ", rawSoc, rawVCell);
    else
        Serial.print("[raw] read FAILED  ");

    if (gaugeBegun)
        Serial.printf("[library] SOC=%.2f%% VCELL=%.4fV\n", gauge.cellPercent(), gauge.cellVoltage());
    else
        Serial.println("[library] not begun");
}
