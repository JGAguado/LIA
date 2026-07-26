# Drivers

## ImuMotionDriver

Talks to the LSM6DSOXTR IMU (`lia_v1` only) directly via `Adafruit_LSM6DSOX`,
bypassing Meshtastic's own `AccelerometerThread`/motion-sensor support:
`ScanI2CTwoWire` resolves this chip's `WHO_AM_I` (`0x6C`) to its `QMI8658`
device type, which `AccelerometerThread` has no `case` for at all -- it
silently falls through to `disable()`, so stock Meshtastic never actually
drives this sensor. `Adafruit_LSM6DSOX` is the correct match instead: `0x6C`
is exactly that class's expected chip ID.

Owned by `CommandService` (not a Meyer's singleton like `LiaBoard` -- nothing
else needs to reach it), which starts/stops it on the `IMU ON`/`IMU OFF`
commands (see `../services/README.md`). Arms a real hardware interrupt on
`LIA_PIN_IMU_INT` rather than polling the sensor's status register: the ISR
only sets a flag (I2C isn't interrupt-safe), consumed from a normal task
context to send the actual "Activity detected" mesh message.

## What's still handled elsewhere

- **SX1262 radio** and **SAM-M10Q GNSS** -- stock Meshtastic behaviour,
  switched on purely by the pin `#define`s in
  [`../meshtastic/variants/esp32s3/lia_v1/variant.h`](../meshtastic/variants/esp32s3/lia_v1/variant.h).
  Meshtastic already contains a full SX1262 (RadioLib) and NMEA/UBX GPS
  stack.
- **Peripheral power, status LED, mode switch, charger status** -- owned by
  [`../board/LiaBoard.*`](../board/README.md), since none of them are stock
  Meshtastic concepts but all are simple enough (plain GPIO/PWM) not to
  warrant a separate driver class.
- **MAX17048 battery gauge** -- no dedicated driver class; `CommandService`
  reads it directly with `Adafruit_I2CDevice`/`Adafruit_BusIO_Register` (see
  `../services/README.md` for why, and `../tools/battery_test/` for the
  standalone test that isolated a real reading bug).

If a future phase needs a peripheral that doesn't fit either of those
buckets, it belongs here as its own subdirectory with a `README.md`,
`examples`, and `tests`, per `firmware/AGENTS.md`'s repository layout.
