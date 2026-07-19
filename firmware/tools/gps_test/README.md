# GPS bring-up test

Standalone SAM-M10Q test for LIA v1 -- no Meshtastic, no LoRa, no LED.
Powers the GNSS module (PPC high) and prints whatever
[TinyGPSPlus](https://github.com/mikalhart/TinyGPSPlus) has decoded over
native-USB serial (115200 baud) every 10 seconds. Deliberately independent of
`firmware/meshtastic/` -- this is a plain Arduino sketch, not a Meshtastic
build, so it has no dependency on a Meshtastic checkout.

Useful for isolating GPS antenna/wiring/fix-time questions (open-sky cold fix,
sensitivity to obstruction) from the rest of the tracker firmware.

## Build & flash

From this directory (`firmware/tools/gps_test/`), with the board connected:

```powershell
pio run -e lia_v1 -t upload --upload-port <PORT>
```

## Reading the output

```
t=40s fix=no sats=4 hdop=-1.0 charsProcessed=1832 sentencesWithFix=0 failedChecksum=0
t=50s fix=yes sats=7 hdop=1.2 lat=41.123456 lon=2.123456 alt=45.3m age=87ms utc=2026-07-19_17:32:04 charsProcessed=2417 sentencesWithFix=3 failedChecksum=0
```

- `charsProcessed` staying near 0 forever -> the module isn't talking at all
  (check wiring/baud, `LIA_PIN_PPC`/GPS UART pins in `firmware/AGENTS.md`).
- `charsProcessed` climbing but `sentencesWithFix` stuck at 0 -> talking fine,
  just no fix yet (needs open sky, or more time for a cold fix).
- `failedChecksum` climbing -> noisy/wrong wiring or wrong baud rate.
