---
title: LoRa
description: Radio hardware and regional configuration for LIA's LoRa link.
---

LIA uses a Semtech SX1262 LoRa transceiver embedded on a [Seeed Studio Wio-SX1262](https://www.seeedstudio.com/Wio-SX1262-LF-Wireless-Module-p-6659.html) with an IPEX port for an external antenna — see [PCB](/LIA/docs/hardware/pcb/#antennas).

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

:::danger
Transmitting outside your region's permitted frequency plan can be illegal. Always set the correct region for your country before use.
:::

## Range expectations

Real-world LoRa range depends heavily on terrain, antenna placement, and the modem preset (spreading factor/bandwidth tradeoff between range and airtime). Open, line-of-sight rural terrain can reach several kilometers node-to-node; dense urban or forested environments will be far shorter.

See also: [Meshtastic Configuration](/LIA/docs/firmware/meshtastic-configuration/).
