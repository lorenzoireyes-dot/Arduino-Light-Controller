# Programmable Light Controller

### Embedded C++ | Arduino Uno R3 | ATmega328P

A standalone embedded light controller developed in C++ using the Arduino framework. The project demonstrates finite state machine design, nonblocking timing, GPIO control, interrupt assisted input processing, UART communication, and nonvolatile configuration storage.

The controller supports four operating modes and can operate independently using battery power.

## Features

- Four operating modes: OFF, ON, BLINK, and TIMED
- Finite state machine with explicit state transitions
- Nonblocking timing using `millis()`
- External pushbutton input with software debouncing
- Short press mode selection and long press reset
- UART serial command interface
- EEPROM storage for operating mode persistence
- Modular functions separating input handling, state transitions, and output control

## Hardware

| Component | Description |
|---|---|
| Microcontroller | Arduino Uno R3 (ATmega328P) |
| Output | Built in LED, digital pin 13 |
| Input | External momentary pushbutton, digital pin 2 |
| Power | USB or external 9V battery through VIN and GND |
| Development | Arduino IDE 2, Embedded C++ |

The button connects between D2 and GND using the microcontroller's internal resistor.

## Operating Modes

| Mode | Behavior |
|---|---|
| OFF | LED remains off |
| ON | LED remains illuminated |
| BLINK | LED toggles every 500 milliseconds |
| TIMED | LED illuminates for 10 seconds, then automatically returns to OFF |

A short button press advances to the next mode. A long press of 1.5 seconds requests OFF.

The automatic TIMED to OFF transition is an intentional design decision.

## Software Architecture

The firmware separates three primary responsibilities:

1. **Input handling:** Processes UART commands and debounces the physical pushbutton.
2. **State management:** Controls operating mode transitions and initializes state-specific variables.
3. **Output control:** Updates the LED based on the current operating mode and elapsed time.

The interrupt service routine records button activity, while button state validation occurs in the main loop.

No blocking `delay()` calls are used.

## Serial Commands

| Command | Action |
|---|---|
| 0 | OFF |
| 1 | ON |
| 2 | BLINK |
| 3 | TIMED |
| n | Next mode |
| s | Display current mode |
| h | Display help |

Serial communication operates at 9600 baud.

## Build and Upload

1. Install Arduino IDE 2.
2. Open `arduino-framework/ProgrammableLightController/ProgrammableLightController.ino`.
3. Select Arduino Uno as the target board.
4. Select the connected board's serial port.
5. Click Verify to compile.
6. Click Upload to program the microcontroller.

## Validation

The following behaviors have been observed on physical hardware:

- Successful firmware compilation and upload
- Pushbutton controlled mode transitions
- LED output control
- Automatic transition from TIMED to OFF
- Standalone operation using an external 9V battery
- Precise timeing
- EEPROM recovery
- Long press handling
- Rapid input behavior

## Comparative Firmware Study

A second implementation is planned using register level AVR C++ without Arduino's hardware abstraction functions.

The comparison will examine:

- Flash and SRAM utilization
- Peripheral configuration
- Interrupt handling
- Timing implementation
- Execution overhead
- Maintainability and portability

Both implementations will target the same ATmega328P hardware and functional requirements.

## Project Status

**Arduino framework version:** Implemented, with hardware validation in progress.

**Register level AVR version:** Planned.

## License

License to be determined.
