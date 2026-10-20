# Standalone AVR-GCC Light Controller

**ATmega328P · Arduino Uno R3 · AVR-G++ · No Arduino core**

This is the third implementation of the [Programmable Light Controller](../README.md). It recreates the Arduino framework and AVR register controllers as a standalone embedded C++ application with an `int main(void)` entry point, built directly with AVR-G++ and uploaded through the Arduino Uno R3 bootloader using AVRDUDE.

## Hardware

- **Target:** Arduino Uno R3, ATmega328P at 16 MHz
- **LED:** Onboard D13 / PB5
- **Pushbutton:** D2 / PD2 / INT0 connected to GND, internal pull-up enabled
- **Serial:** USB serial via USART0, 9600 baud

## Firmware features

| Feature | Implementation |
|---|---|
| GPIO | Direct `DDRB`, `PORTB`, `DDRD`, `PORTD`, `PIND` register access |
| Timebase | Timer1 in CTC mode, 1 ms interrupt |
| Interrupts | Timer1 compare-match ISR and INT0 falling edge ISR |
| Button | 40 ms polling debounce; 1.5 s long press resets to OFF |
| Controller | Nonblocking finite state machine |
| Serial | USART0 transmit/receive via hardware registers |
| Persistence | Direct EEPROM access at address 0, avoiding redundant writes |

INT0 records button activity; actual button debouncing and mode changes are processed by polling the input in the main loop.

## Operating modes

| Mode | Behavior |
|---|---|
| OFF | LED off |
| ON | LED on |
| BLINK | LED toggles every 500 ms |
| TIMED | LED on for 10 seconds, then returns to OFF |

Short button presses advance to the next mode. Automatic TIMED expiration does not overwrite the saved EEPROM mode.

**Serial commands (9600 baud):** `0` OFF, `1` ON, `2` BLINK, `3` TIMED, `n` next mode, `s` status, `h` help.

## Building on Windows

This version is built without Arduino IDE's compile pipeline. The AVR toolchain installed with Arduino IDE can be used directly from PowerShell.

From the directory containing `AVR-Standalone.cpp`, with `$AVR` set to the AVR-GCC `bin` directory:

```powershell
& "$AVR\avr-g++.exe" -mmcu=atmega328p -DF_CPU=16000000UL -Os -std=gnu++11 -Wall -Wextra -o firmware.elf AVR-Standalone.cpp
& "$AVR\avr-objcopy.exe" -O ihex -R .eeprom firmware.elf firmware.hex
& "$AVR\avr-size.exe" -C --mcu=atmega328p firmware.elf
```

Upload `firmware.hex` using AVRDUDE with the Uno R3 bootloader (Arduino programmer protocol, typically 115200 baud, using the correct COM port). The bootloader remains on the board, but the application itself does not link the Arduino core.

## Build measurements

Measured for the complete standalone firmware targeting the ATmega328P with AVR-G++ and `-Os`:

| Memory | Usage |
|---|---:|
| Flash | **1,762 bytes (5.4%)** |
| Static SRAM (`.data + .bss + .noinit`) | **256 bytes (12.5%)** |

These figures come from `avr-size -C --mcu=atmega328p firmware.elf`. The static SRAM figure does not include peak stack usage.

## Verification

The complete firmware was compiled, converted to Intel HEX, and uploaded successfully to the Arduino Uno R3 with AVRDUDE. The physical button and serial-command interface have been tested on hardware, with the same controller behavior as the earlier implementations.

**Demonstration video:** [Standalone AVR-GCC hardware demonstration](media/StandaloneAVR_Demo.mp4).

## Memory comparison

See the completed [three-way flash and SRAM comparison](../COMPARISON.md) for measured results and compilation evidence.
