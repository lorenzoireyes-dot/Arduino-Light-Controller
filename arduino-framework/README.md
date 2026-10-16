# Arduino Framework Implementation

This is the original programmable light controller written in embedded C++ using the Arduino framework on the Arduino Uno R3 (ATmega328P).

## Design

- Arduino GPIO functions control the onboard LED and read the external pushbutton.
- A four-state finite state machine implements OFF, ON, BLINK (500 ms), and TIMED (10 seconds).
- `millis()` supports nonblocking operation, 40 ms button debouncing, and 1.5-second long-press reset.
- `attachInterrupt()` detects button activity; input validation occurs in the main loop.
- `Serial` provides a 9600-baud command interface (`0`, `1`, `2`, `3`, `n`, `s`, `h`).
- `EEPROM.update()` stores the selected mode and reduces redundant writes.

The TIMED mode returns to OFF after 10 seconds without overwriting the stored user selection.

## Hardware

Arduino Uno R3; onboard LED on D13; momentary pushbutton between D2 and GND with internal pull-up.

## Build

Open the framework implementation's `.ino` sketch in Arduino IDE 2, select **Arduino Uno** and the correct port, then Verify and Upload.

## Related implementation

See [AVR register-level implementation](../avr-register-level/README.md) for the same controller implemented using direct hardware register access.
