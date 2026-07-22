#include "ImuMotionDriver.h"

#include "configuration.h"

#include <Adafruit_LSM6DSOX.h>
#include <esp_attr.h>

namespace
{
// Confirmed via the boot-time I2C scan log ("QMI8658 found at address
// 0x6b") -- not this sensor family's 0x6A default address.
constexpr uint8_t kImuI2cAddress = 0x6B;
constexpr uint8_t kWakeDurationSamples = 1;
constexpr uint8_t kWakeThreshold = 20;

Adafruit_LSM6DSOX sensor;
} // namespace

volatile bool ImuMotionDriver::activityPending_ = false;

void IRAM_ATTR ImuMotionDriver::onInterrupt()
{
    activityPending_ = true;
}

bool ImuMotionDriver::begin()
{
    if (began_)
        return true;

    if (!sensor.begin_I2C(kImuI2cAddress))
        return false;

    sensor.setAccelRange(LSM6DS_ACCEL_RANGE_2_G);
    sensor.enableWakeup(true, kWakeDurationSamples, kWakeThreshold);
    sensor.configInt1(false, false, false, false, true); // route the wake-up event to INT1

    pinMode(LIA_PIN_IMU_INT, INPUT);
    began_ = true;
    setInterruptEnabled(true);
    return true;
}

void ImuMotionDriver::setInterruptEnabled(bool enabled)
{
    if (!began_)
        return;

    if (enabled) {
        activityPending_ = false;
        attachInterrupt(digitalPinToInterrupt(LIA_PIN_IMU_INT), onInterrupt, RISING);
    } else {
        detachInterrupt(digitalPinToInterrupt(LIA_PIN_IMU_INT));
    }
}

bool ImuMotionDriver::consumeActivityDetected()
{
    if (!activityPending_)
        return false;

    activityPending_ = false;
    sensor.awake(); // Reading WAKEUP_SRC clears the sensor's own latch.
    return true;
}
