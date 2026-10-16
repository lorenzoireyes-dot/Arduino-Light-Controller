# Programmable Light Controller

**Arduino Uno R3 · ATmega328P · Embedded C++**

Two implementations of the same programmable LED controller: one built with the Arduino framework and one using direct AVR peripheral registers. Both have been tested on physical hardware.

## Implementations

| Implementation | Approach | Status |
|---|---|---|
| [Arduino framework](arduino-framework/README.md) | Arduino GPIO, timing, serial, interrupt, and EEPROM APIs | Functional; hardware tested |
| [AVR register-level](avr-register-level/README.md) | Direct GPIO, Timer1, INT0, USART0, and EEPROM registers | Functional; hardware tested |

**Note:** The AVR register-level version is currently compiled as an Arduino `.ino` sketch using `setup()` and `loop()`. A standalone AVR-GCC `main()` build is planned; the Arduino core has not yet been removed.

## Controller behavior

| Mode | LED behavior |
|---|---|
| OFF | Off |
| ON | Continuously on |
| BLINK | Toggles every 500 ms |
| TIMED | On for 10 seconds, then automatically returns to OFF |

A short press of the button advances to the next mode. Holding it for 1.5 seconds resets to OFF. Input debouncing uses a 40 ms stable-state interval. User-selected modes are stored in EEPROM and restored at startup; automatic TIMED expiration does not overwrite the stored selection.

## Hardware

- **Board:** Arduino Uno R3, ATmega328P at 16 MHz
- **LED:** Onboard LED, D13 / PB5
- **Button:** Momentary switch between D2 / PD2 / INT0 and GND, using the internal pull-up
- **Communication:** USB serial, 9600 baud
- **Power:** USB or an external battery supply through the board's appropriate power inputs

## Serial commands

| Command | Action |
|---|---|
| `0` | OFF |
| `1` | ON |
| `2` | BLINK |
| `3` | TIMED |
| `n` | Next mode |
| `s` | Current status |
| `h` | Help |

## Hardware demonstration

A demonstration video is part of the project. See the repository files for the uploaded media.

## Development and comparison

Both implementations use the same controller requirements so their architectures can be compared. The register-level version configures peripherals directly; the framework version uses Arduino APIs. Planned work includes a standalone AVR-GCC build and measured comparisons of flash usage, SRAM usage, execution timing, and maintainability.

**Verification:** Mode selection, LED timing, button operation, serial communication, and EEPROM persistence have been exercised on the Arduino Uno R3. No benchmark results are claimed yet.
