---
title: Accelerometer
description: Motion detection with the LSM6DSOXTR IMU, and the IMU ON/OFF command.
---

LIA carries an **LSM6DSOXTR** 6-axis IMU (accelerometer + gyroscope) on the I2C bus (`SDA`/`SCL`), with its interrupt line wired directly to the ESP32-S3 on `IMU_INT` (GPIO6 — see [Microcontroller](/LIA/docs/firmware/microcontroller/)). Both the V1.0 and V1.1 boards carry the same reworked IMU wiring and the same `lia_v1` firmware — see [Building the Firmware](/LIA/docs/firmware/building-the-firmware/#which-variant).

:::note
Meshtastic's own I2C device scan identifies this chip under the label `QMI8658` rather than `LSM6DS3`/`LSM6DSOXTR` — its `WHO_AM_I` register (`0x6C`) is the LSM6DSOX family's real chip ID, but stock Meshtastic's sensor-detection table resolves that specific ID/address pair to its `QMI8658` entry instead. The physical part is genuinely the LSM6DSOXTR; this is just how it gets labeled in the boot log. Stock Meshtastic's own accelerometer driver has no handling for that label at all, so the firmware talks to the chip directly instead (`Adafruit_LSM6DSOX`, matched to that same `0x6C` chip ID) rather than relying on stock support.
:::

## Motion-triggered activity messages

Unlike a continuously-running motion sensor, the IMU here is opt-in and reported over the mesh rather than used to skip the tracker's own sleep schedule:

- Send **`IMU ON`** as a direct message to the device (see [Meshtastic Configuration](/LIA/docs/firmware/meshtastic-configuration/)) to arm it. The first activation configures the sensor's wake-up detector and its hardware interrupt on `IMU_INT`; if the sensor doesn't respond, the device replies `"IMU not detected"` instead.
- Once armed, a real motion event sends **`"Activity detected"`** to the configured target node.
- Send **`IMU OFF`** to disarm it again.

The interrupt itself only sets a flag — reading the sensor over I2C isn't safe to do from inside an interrupt handler, so the actual mesh message is sent from the firmware's normal periodic task loop shortly after, not instantaneously from the interrupt.

:::note
Using this to skip a scheduled wake-and-broadcast cycle entirely when stationary (rather than just reporting activity as its own message) hasn't been implemented yet.
:::

See also: [Meshtastic Configuration](/LIA/docs/firmware/meshtastic-configuration/), [Power Management](/LIA/docs/firmware/power-management/), and [Microcontroller](/LIA/docs/firmware/microcontroller/).
