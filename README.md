# esp32-pzem004t-power-monitor-mqtt — publish voltage, current, power and energy from a PZEM-004T over MQTT

IoT example by [Fluidwire](https://fluidwire.com/).

Sample sketch for an ESP32 dev board that reads **voltage, current, active power, accumulated energy (kWh), frequency and power factor** from a **PZEM-004T v3** AC meter module over UART and publishes the values as JSON to an MQTT broker.

It's a good building block for household electricity-bill studies and appliance-monitoring thesis prototypes in the Philippines, where you want to see how many kWh a refrigerator, aircon or water pump really uses.

## What it does

Every 5 seconds it polls the PZEM-004T (Modbus-RTU over UART, handled by the `PZEM004Tv30` library) and publishes one JSON message. Energy is accumulated **inside the module**, so the kWh total survives ESP32 resets and power cuts. If the module does not answer, the node publishes an error message instead of bogus zeros.

## Required hardware

- 1x ESP32 dev board
- 1x PZEM-004T v3 module with its split-core current transformer (CT)
- An MQTT broker (local Mosquitto, or a public test broker like `broker.hivemq.com`)
- A 2.4 GHz WiFi network
- An insulated enclosure and a fuse or breaker on the AC side

## Required libraries

Install via Library Manager:

- **PubSubClient** by Nick O'Leary
- **PZEM004Tv30** by Jakub Mandula

## Wiring (UART2)

| PZEM-004T pin | ESP32 pin       |
|---------------|-----------------|
| 5V            | VIN (5V)        |
| GND           | GND             |
| TX            | GPIO16 (RX2)    |
| RX            | GPIO17 (TX2)    |

The AC side of the module (L/N terminals and the CT clamp) connects to the mains load you want to measure. The TTL side is opto-isolated from the AC side.

## Mains safety

**This project involves mains voltage (230 V in the Philippines), which can kill.** Only wire the AC terminals with the supply disconnected, use an insulated enclosure, fuse the circuit, never touch the module while powered, and have a qualified electrician do the AC wiring if you are not trained. Clamp the CT around a **single** conductor (live *or* neutral), never both.

## Setup

1. Copy `config.h.example` → `config.h`. Fill in your WiFi + MQTT broker details.
2. Wire the TTL side to the ESP32 as above (AC power off).
3. Wire the AC side and clamp the CT around one conductor.
4. Flash. Open the Serial Monitor at **115200 baud**.

## Expected serial output

```
[pzem] PZEM-004T v3 on Serial2
[wifi] connecting to my-network
....
[wifi] IP: 192.168.1.42
[mqtt] connecting... ok
[mqtt] {"voltage_v":229.8,"current_a":0.412,"power_w":86.3,"energy_kwh":12.345,"freq_hz":60.0,"pf":0.91}
```

Subscribe from your machine to watch the data arrive:

```
mosquitto_sub -h broker.hivemq.com -t "fluidwire/demo/pzem"
```

## Troubleshooting

- **`[pzem] no response`** — check TX/RX are crossed (PZEM TX → GPIO16), the module has 5V, and the AC side is energised (the module is powered from the mains sensing side's voltage reading for measurements).
- **Current always 0** — the CT is not clamped around a conductor carrying load, or it is clamped around both live and neutral.
- **Energy keeps growing between tests** — the total is stored in the module. The library offers `pzem.resetEnergy()` if you want to zero it.
- **`failed state=-2`** on MQTT — the broker host/port is wrong or unreachable.
- **Frequency shows 0.0** — the module returned an invalid frequency reading; the sketch reports 0.0 rather than NaN.

## Security note

This sketch keeps your WiFi and MQTT credentials in `config.h`, which is `.gitignore`'d — it never gets committed. Only `config.h.example` ships in the repo. Never hardcode real SSID/password into the `.ino`. For a public broker, anyone subscribed to your topic can see your electricity usage; use a private broker with `MQTT_USER`/`MQTT_PASSWORD` (and TLS — see [esp32-mqtt-tls-secure](https://github.com/fluidwire/esp32-mqtt-tls-secure)) for anything sensitive.

## License

MIT — see [LICENSE](LICENSE).

## Got stuck?

Drop by [fluidwire.com](https://fluidwire.com/) — happy to help.
