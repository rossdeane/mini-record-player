# Mini record player NFC reader

ESP32 sketch for the mini record player: reading an NFC tag publishes its UID over MQTT; removing it publishes `removed`. The single NeoPixel shows red while disconnected, green when ready, and breathing blue while a tag is present. Playback is handled by your own MQTT subscriber, not by this sketch.

## 3D-printed record player

The enclosure and mini vinyls are not my design. They are [geroulas](https://makerworld.com/en/@geroulas)'s [ESP32 NFC Mini Record Player](https://makerworld.com/en/models/1980570-esp32-nfc-mini-record-player-vinyl-tag-reader) on MakerWorld. It is a lovely snap-fit player: platter, tonearm, and little vinyls you stick NFC tags on, and it looks the part sitting on a shelf. This repo is only the firmware for my own build of that model. Download and print theirs.

## Hardware

- ESP32-WROOM-32 Development Board
- NFC RFID Module PN532 (set to I2C mode)
- LED SMD 5050 (WS2812/NeoPixel compatible)
- NFC/RFID Stickers (25mm)

## Software

- Arduino IDE with the ESP32 board package.
- Arduino libraries: **Adafruit PN532**, **Adafruit NeoPixel**, and **PubSubClient** (plus any dependencies requested by the library manager).

| Connection | ESP32 pin |
| --- | --- |
| PN532 SDA | GPIO 21 |
| PN532 SCL | GPIO 22 |
| NeoPixel data | GPIO 4 |

Connect power and ground appropriate to your particular modules and ESP32 board. These are the pins used in my build; change the constants near the top of the sketch if yours differ.

## Setup

1. Copy `mini_record_player/config.example.h` to `mini_record_player/config.h`.
2. Fill in Wi-Fi and MQTT host, port, topic, username and password. If your MQTT broker allows anonymous access, use empty strings for both MQTT credentials.
3. Open `mini_record_player/mini_record_player.ino` in Arduino IDE, select your ESP32 board and port, then upload. The serial monitor runs at **115200 baud**.
4. Subscribe to `recordplayer/tag` (or your chosen topic). A tag sends its lowercase hexadecimal UID with leading zeroes; removing it sends `removed`. Messages are retained, and the current state is published again after MQTT reconnects.

For example, a subscriber can map a known UID to an album and stop playback when it receives `removed`. This sketch only reads ISO 14443A tags supported by the PN532 library. Its UID is an identifier for your setup, not proof that a tag is authentic.

`config.h` is excluded by `.gitignore`. Keep local credentials out of commits and rotate any credentials that have already been shared.