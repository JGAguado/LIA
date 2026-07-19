// Standalone SAM-M10Q bring-up test for LIA v1.
//
// No Meshtastic, no LoRa, no LED -- just powers the GNSS module (PPC high,
// per firmware/AGENTS.md's hardware constraint) and prints whatever
// TinyGPSPlus has decoded over native-USB serial every 10 seconds. Useful
// for isolating GPS antenna/wiring/fix-time questions from the rest of the
// tracker firmware's complexity.
//
// Pin numbers match firmware/AGENTS.md's corrected pinout (verified against
// hardware/kicad/LIA.kicad_sch) -- kept independent of firmware/meshtastic/
// on purpose, so this has no dependency on a Meshtastic checkout.

#include <Arduino.h>
#include <TinyGPSPlus.h>

namespace {
constexpr int kPinPPC = 21;
constexpr int kGpsTxPin = 17; // ESP32-S3 -> GNSS RX
constexpr int kGpsRxPin = 18; // ESP32-S3 <- GNSS TX
constexpr uint32_t kGpsBaud = 9600;
constexpr uint32_t kReportIntervalMs = 10000;

TinyGPSPlus gps;
HardwareSerial gpsSerial(1);
uint32_t lastReportMs = 0;
} // namespace

void setup()
{
    Serial.begin(115200);
    delay(1500); // let the native-USB CDC host enumerate before we print anything

    Serial.println("LIA GPS test -- no Meshtastic, no LoRa, no LED, just serial every 10s");

    pinMode(kPinPPC, OUTPUT);
    digitalWrite(kPinPPC, HIGH); // PPC must be high before the SAM-M10Q is touched
    delay(100);                 // let the peripheral rail settle

    gpsSerial.begin(kGpsBaud, SERIAL_8N1, kGpsRxPin, kGpsTxPin);
}

void loop()
{
    while (gpsSerial.available())
        gps.encode(gpsSerial.read());

    uint32_t now = millis();
    if (now - lastReportMs < kReportIntervalMs)
        return;
    lastReportMs = now;

    Serial.printf("t=%lus fix=%s sats=%d hdop=%.1f", now / 1000, gps.location.isValid() ? "yes" : "no",
                  gps.satellites.isValid() ? gps.satellites.value() : -1, gps.hdop.isValid() ? gps.hdop.hdop() : -1.0);

    if (gps.location.isValid()) {
        Serial.printf(" lat=%.6f lon=%.6f alt=%.1fm age=%lums", gps.location.lat(), gps.location.lng(),
                      gps.altitude.isValid() ? gps.altitude.meters() : 0.0, gps.location.age());
    }
    if (gps.time.isValid() && gps.date.isValid()) {
        Serial.printf(" utc=%04d-%02d-%02d_%02d:%02d:%02d", gps.date.year(), gps.date.month(), gps.date.day(),
                      gps.time.hour(), gps.time.minute(), gps.time.second());
    }

    // Raw parser stats -- distinguishes "GPS isn't talking at all" (charsProcessed
    // stays ~0: check wiring/baud) from "talking but no fix yet" (charsProcessed
    // climbs, sentencesWithFix stays 0: needs open sky / more time) from a
    // wiring/noise problem (failedChecksum climbing).
    Serial.printf(" charsProcessed=%lu sentencesWithFix=%lu failedChecksum=%lu\n", gps.charsProcessed(),
                  gps.sentencesWithFix(), gps.failedChecksum());
}
