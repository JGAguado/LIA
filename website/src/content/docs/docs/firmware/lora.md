---
title: LoRa
description: Radio hardware and regional configuration for LIA's LoRa link.
---

LIA uses a Semtech SX1262 LoRa transceiver with an external FPC antenna, following the same regional frequency plans as Meshtastic itself.

## Region settings

Meshtastic requires a region to be set before the radio will transmit. Set it to match your local regulations:

| Region code | Typical use |
| --- | --- |
| `US` | United States, Canada |
| `EU_868` | European Union |
| `EU_433` | Parts of Europe (433 MHz band) |
| `ANZ` | Australia, New Zealand |

```sh
meshtastic --set lora.region EU_868
```

> [!CAUTION]
> Transmitting outside your region's permitted frequency plan can be illegal. Always set the correct region for your country before use.

## Range expectations

Real-world LoRa range depends heavily on terrain, antenna placement, and the modem preset (spreading factor/bandwidth tradeoff between range and airtime). Open, line-of-sight rural terrain can reach several kilometers node-to-node; dense urban or forested environments will be far shorter.

See also: [Meshtastic Configuration](/LIA/docs/firmware/meshtastic-configuration/).
