# AVR Register Level Implementation

This implementation recreates the Arduino light controller using direct ATmega328P register access in C++. It has been tested on an Arduino Uno R3.

**Build distinction:** The current source is an Arduino `.ino` sketch and still uses Arduino's startup infrastructure. It does not use Arduino's GPIO, timing, serial, or EEPROM APIs for the controller logic. A standalone AVR-GCC `main()` version is planned.

## Peripherals and implementation

| Feature | AVR implementation |
|---|---|
| LED output | `DDRB`, `PORTB` (PB5 / D13) |
| Button input | `DDRD`, `PORTD`, `PIND` (PD2 / D2) |
| External interrupt | INT0 configured through `EICRA`, `EIMSK`, `EIFR` |
| System timebase | Timer1 CTC compare interrupt, 1 ms tick |
| Debounce | Nonblocking 40 ms stable-state sampling in the main loop |
| Long press | 1.5-second hold resets to OFF |
| Serial | USART0 registers, 9600 baud, transmit and receive |
| Persistence | EEPROM address 0 via `EEAR`, `EEDR`, `EECR` |

INT0 records button activity; the actual debouncing and release detection use polling in the main loop. A 32-bit millisecond counter is read atomically to avoid torn reads.

## Modes and controls

- **OFF:** LED off
- **ON:** LED on
- **BLINK:** LED toggles every 500 ms
- **TIMED:** LED on for 10 seconds, then automatically OFF

Short press cycles modes; long press resets to OFF. Serial commands: `0`, `1`, `2`, `3`, `n`, `s`, `h`. Mode changes are reported over serial. User selected modes persist across resets and power loss. TIMED automatic expiration does not overwrite the stored selection.

## Hardware and testing

Arduino Uno R3 (16 MHz ATmega328P), onboard D13 LED, and a pushbutton wired from D2 to GND using the internal pull up. Tested with Arduino IDE 2 and its Serial Monitor.

## Next steps

Move the source into a standalone AVR-GCC project with `main()`, then measure flash, SRAM, and timing against the [Arduino framework implementation](../arduino-framework/README.md).
