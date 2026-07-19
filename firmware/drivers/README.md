# Drivers

Empty by design, for now. Every peripheral currently in scope is handled
without bespoke driver code here:

- **SX1262 radio** and **SAM-M10Q GNSS** -- stock Meshtastic behaviour,
  switched on purely by the pin `#define`s in
  [`../meshtastic/variants/esp32s3/lia_v1/variant.h`](../meshtastic/variants/esp32s3/lia_v1/variant.h).
  Meshtastic already contains a full SX1262 (RadioLib) and NMEA/UBX GPS stack.
- **Peripheral power, status LED, mode switch, charger status** -- owned by
  [`../board/LiaBoard.*`](../board/README.md), since none of them are stock
  Meshtastic concepts but all are simple enough (plain GPIO/PWM) not to
  warrant a separate driver class.
- **I2C bus (IMU, fuel gauge)** -- out of scope entirely on the current PCB
  revision; see `firmware/AGENTS.md` "Hardware Constraints".

If a future phase needs a peripheral that doesn't fit either of those
buckets, it belongs here as its own subdirectory with a `README.md`,
`examples`, and `tests`, per `firmware/AGENTS.md`'s repository layout.
