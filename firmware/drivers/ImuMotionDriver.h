#pragma once

/// Wraps the LSM6DSOXTR IMU directly via Adafruit_LSM6DSOX, bypassing
/// Meshtastic's own AccelerometerThread/LSM6DS3Sensor: those only
/// instantiate for ScanI2C::DeviceType::LSM6DS3, but this chip's WHO_AM_I
/// (0x6C) makes ScanI2C label it "QMI8658" instead (see
/// firmware/board/README.md "Open questions") -- a device type
/// AccelerometerThread has no case for at all, so stock Meshtastic never
/// drives it.
///
/// Not a Meyer's singleton like LiaBoard -- nothing besides CommandService
/// needs to reach it, so CommandService just owns one as a plain member and
/// starts/stops it on IMU ON/IMU OFF.
class ImuMotionDriver
{
  public:
    /// Starts I2C comms, configures the wake-up detector, and arms the
    /// hardware interrupt on LIA_PIN_IMU_INT. Returns false if the sensor
    /// doesn't respond. Safe to call again after the first success (no-op).
    bool begin();

    /// Re-attaches (true) or detaches (false) the GPIO interrupt. begin()
    /// must have succeeded first; does nothing otherwise. Does not touch
    /// the sensor's own wake-up configuration -- only whether the ESP32
    /// side is listening for it.
    void setInterruptEnabled(bool enabled);

    /// True once, the first call after the interrupt has fired since the
    /// last call -- clears both the pending flag and the sensor's own
    /// WAKEUP_SRC latch. Safe to call from a normal task context; the ISR
    /// itself only ever touches the pending flag (Wire isn't
    /// interrupt-safe, so the I2C clear has to happen here instead).
    bool consumeActivityDetected();

  private:
    static void onInterrupt();

    static volatile bool activityPending_;

    bool began_ = false;
};
