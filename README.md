# ESP32 OLED Lyrics Visualizer

Synchronized lyric visuals on a 0.96" SSD1306 OLED, driven by an ESP32, with four reactive accent LEDs.
Each word appears with its own text effect (pop, shake, invert, bounce, typewriter, zoom) on top of nine
animated backgrounds that change as the song moves along.

Current song: **Dracula**

## Demo

Add your Instagram reel link, a GIF or a photo of the build here.

## Features

- 9 full-screen animations: Blood Drip Portal, Hypno Spiral, Star Warp, Plasma Tunnel + EQ, DNA Helix,
  Particle Explosion, Glitch Scanlines, Tesla Coil Arc, Mobius Rings
- Per-word text effects: pop, shake, invert, glitch, zoom-in, typewriter, bounce
- 4 PWM LEDs with a different reactive pattern for every animation
- Lyric timing lives in `lyrics.h`, so a new song only needs a new timeline
- Auto-detects the OLED I2C address (0x3C, then 0x3D)
- Loops automatically

## Hardware

| Part | Notes |
|------|-------|
| ESP32 Dev Module (38-pin) | |
| 0.96" OLED, SSD1306, 128x64, I2C | |
| 4x LEDs | any colour |
| 4x 220 ohm resistors | one per LED |
| Breadboard + jumper wires | |

## Wiring

| OLED | ESP32 |
|------|-------|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 19 |
| SCL | GPIO 18 |

| LED (anode, via 220 ohm) | ESP32 |
|--------------------------|-------|
| LED1 | GPIO 25 |
| LED2 | GPIO 26 |
| LED3 | GPIO 32 |
| LED4 | GPIO 33 |

LED cathodes go to GND. Pins can be changed at the top of `dracula-song.ino`.

## Software setup

1. Install the [Arduino IDE](https://www.arduino.cc/en/software).
2. Install the ESP32 board package **version 3.x** (Boards Manager, search "esp32" by Espressif).
   Version 3.x is required because the sketch uses `analogWrite`.
3. Install these libraries (Library Manager):
   - Adafruit GFX Library
   - Adafruit SSD1306
4. Open `dracula-song/dracula-song.ino`. The folder name must match the `.ino` name, and `lyrics.h` must sit in the same folder.
5. Select **ESP32 Dev Module**, choose your port, and upload.
6. Start the song and press the board's EN (reset) button at the same moment so the lyrics line up.

## How the timing works

Each entry in `lyrics.h` is:

```cpp
{ delayBeforeMs, durationMs, "word", EFFECT, textSize, 0 }
```

The start time of every word is calculated from the delays and durations before it. To re-sync, adjust those
numbers. To add a song, copy the folder and write a new timeline.

## Troubleshooting

- **Serial monitor says "OLED I2C address: NOT FOUND":** check SDA/SCL wiring and that VCC is on 3.3V.
  The LEDs will also blink in a loop when the OLED fails to start.
- **Compile error about `analogWrite`:** your ESP32 core is older than 3.x. Update it in Boards Manager.
- **Lyrics drift out of sync:** tweak the `delayBeforeMs` / `durationMs` values in `lyrics.h`.

## Credits

- Original sketch and animation design: **CircuitForge** and **@Afxlabz** ("GOD MODE" upgrade).
  Please check their channel or repository for the original licence before reusing this code.
- Song "Dracula": all rights belong to the original artists and rights holders. This repo contains only the
  timing data for a personal, non-commercial display project.
- Hardware build, wiring and adaptation: *your name here*

## Disclaimer

Educational / hobby project, not affiliated with any artist or label.
