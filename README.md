# Programmable Light Controller

**Arduino Uno R3 · ATmega328P · Embedded C++**

Three implementations of the same programmable LED controller, progressing from Arduino framework APIs to direct AVR register access and a standalone AVR-GCC application. All three have been tested on physical hardware.

## Implementations

| Implementation | Approach | Status |
|---|---|---|
| [Arduino framework](arduino-framework/README.md) | Arduino GPIO, timing, serial, interrupt, and EEPROM APIs | Functional; hardware tested |
| [AVR register-level](avr-register-level/README.md) | Direct GPIO, Timer1, INT0, USART0, and EEPROM registers in an Arduino `.ino` sketch | Functional; hardware tested |
| [Standalone AVR-GCC](avr-standalone/README.md) | Direct registers and a conventional `main()`; no Arduino core | Compiled, uploaded, hardware tested |

**Implementation distinction:** The AVR-register-level `.ino` version still uses Arduino startup infrastructure. The standalone AVR-GCC version uses its own `main()` and does not link the Arduino core.

## Controller behavior

| Mode | LED behavior |
|---|---|
| OFF | Off |
| ON | Continuously on |
| BLINK | Toggles every 500 ms |
| TIMED | On for 10 seconds, then automatically returns to OFF |

A short press of the button advances to the next mode. Holding it for 1.5 seconds resets to OFF. Input debouncing uses a 40 ms interval. User selected modes are stored in EEPROM and restored at startup; automatic TIMED expiration does not overwrite the stored selection.

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

All three implementations use the same controller requirements. The standalone AVR-GCC version compiled to **1,762 bytes of flash and 256 bytes of static SRAM** using `-Os`. A consistent three-way comparison of flash usage, SRAM usage, execution timing, and maintainability is planned.

**Verification:** Mode selection, LED timing, button operation, serial communication, and EEPROM persistence have been exercised on the Arduino Uno R3. Cross-implementation benchmark results are not yet available.
