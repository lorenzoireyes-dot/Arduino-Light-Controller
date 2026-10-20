
#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>

// ============================================================
// Configuration
// ============================================================

constexpr uint32_t DEBOUNCE_MS = 40;
constexpr uint32_t BLINK_MS = 500;
constexpr uint32_t TIMED_MS = 10000;
constexpr uint32_t LONG_PRESS_MS = 1500;

constexpr uint16_t EEPROM_MODE_ADDRESS = 0;

// ============================================================
// Operating Modes
// ============================================================

enum class LightMode : uint8_t
{
    OFF = 0,
    ON = 1,
    BLINK = 2,
    TIMED = 3
};

// ============================================================
// Global State
// ============================================================

volatile uint32_t systemMilliseconds = 0;
volatile bool buttonEventPending = false;

LightMode currentMode = LightMode::OFF;

bool ledState = false;

bool lastRawButtonDown = false;
bool stableButtonDown = false;
bool longPressHandled = false;

uint32_t lastButtonChangeTime = 0;
uint32_t buttonPressStartTime = 0;
uint32_t previousBlinkTime = 0;
uint32_t timedStartTime = 0;

// ============================================================
// Interrupt Service Routines
// ============================================================

// Timer1 compare match: 1 ms system tick
ISR(TIMER1_COMPA_vect)
{
    systemMilliseconds++;
}

// External button interrupt
ISR(INT0_vect)
{
    buttonEventPending = true;
}

// ============================================================
// System Timebase
// ============================================================

uint32_t getMilliseconds()
{
    uint8_t savedSREG = SREG;

    cli();

    uint32_t currentTime = systemMilliseconds;

    SREG = savedSREG;

    return currentTime;
}

void configureTimer1()
{
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    // 16 MHz / 64 / 1000 Hz - 1
    OCR1A = 249;

    // CTC mode, prescaler = 64
    TCCR1B = (1 << WGM12) |
        (1 << CS11) |
        (1 << CS10);

    TIMSK1 = (1 << OCIE1A);
}

// ============================================================
// GPIO
// ============================================================

void configureGPIO()
{
    // PB5 / D13: LED output
    DDRB |= (1 << DDB5);

    // Start with LED off
    PORTB &= ~(1 << PORTB5);

    // PD2 / D2: button input
    DDRD &= ~(1 << DDD2);

    // Enable internal pull-up
    PORTD |= (1 << PORTD2);
}

void setLED(bool enabled)
{
    ledState = enabled;

    if (enabled)
    {
        PORTB |= (1 << PORTB5);
    }
    else
    {
        PORTB &= ~(1 << PORTB5);
    }
}

// ============================================================
// External Interrupt
// ============================================================

void configureButtonInterrupt()
{
    // INT0 falling edge
    EICRA &= ~(1 << ISC00);
    EICRA |= (1 << ISC01);

    // Clear pending interrupt
    EIFR = (1 << INTF0);

    // Enable INT0
    EIMSK |= (1 << INT0);
}

// ============================================================
// USART0 - 9600 Baud
// ============================================================

void configureUSART()
{
    // 16 MHz, 9600 baud, normal speed
    UBRR0H = 0;
    UBRR0L = 103;

    UCSR0A = 0;

    // Enable receiver and transmitter
    UCSR0B = (1 << RXEN0) |
        (1 << TXEN0);

    // 8 data bits, no parity, 1 stop bit
    UCSR0C = (1 << UCSZ01) |
        (1 << UCSZ00);
}

void usartTransmit(char character)
{
    // Wait for transmit buffer
    while (!(UCSR0A & (1 << UDRE0)))
    {
    }

    UDR0 = character;
}

void usartPrint(const char* message)
{
    while (*message != '\0')
    {
        usartTransmit(*message++);
    }
}

void usartPrintLine(const char* message)
{
    usartPrint(message);
    usartPrint("\r\n");
}

bool usartAvailable()
{
    return (UCSR0A & (1 << RXC0)) != 0;
}

char usartRead()
{
    return UDR0;
}

// ============================================================
// EEPROM
// ============================================================

uint8_t eepromReadByte(uint16_t address)
{
    // Wait for any previous EEPROM write
    while (EECR & (1 << EEPE))
    {
    }

    EEAR = address;

    // Start EEPROM read
    EECR |= (1 << EERE);

    return EEDR;
}

void eepromWriteByte(uint16_t address, uint8_t value)
{
    // Avoid unnecessary EEPROM wear
    if (eepromReadByte(address) == value)
    {
        return;
    }

    // Wait until EEPROM is ready
    while (EECR & (1 << EEPE))
    {
    }

    uint8_t savedSREG = SREG;

    cli();

    EEAR = address;
    EEDR = value;

    // EEPROM erase-and-write programming mode
    EECR &= ~((1 << EEPM1) | (1 << EEPM0));

    // The master-enable to program-enable sequence
    // must occur within four CPU clock cycles.
    // Keep these instructions together in assembly.
    __asm__ __volatile__(
        "sbi %[eecr], %[eempe]\n\t"
        "sbi %[eecr], %[eepe]\n\t"
        :
    : [eecr] "I" (_SFR_IO_ADDR(EECR)),
        [eempe] "I" (EEMPE),
        [eepe] "I" (EEPE)
        : "memory"
        );

    SREG = savedSREG;
}

// ============================================================
// Mode Management
// ============================================================

const char* getModeName(LightMode mode)
{
    switch (mode)
    {
    case LightMode::OFF:
        return "OFF";

    case LightMode::ON:
        return "ON";

    case LightMode::BLINK:
        return "BLINK";

    case LightMode::TIMED:
        return "TIMED";

    default:
        return "UNKNOWN";
    }
}

void printCurrentMode()
{
    usartPrint("Current mode: ");
    usartPrintLine(getModeName(currentMode));
}

void setMode(LightMode newMode, bool saveToEEPROM = true)
{
    currentMode = newMode;

    uint32_t now = getMilliseconds();

    previousBlinkTime = now;
    timedStartTime = now;

    switch (currentMode)
    {
    case LightMode::OFF:
        setLED(false);
        break;

    case LightMode::ON:
        setLED(true);
        break;

    case LightMode::BLINK:
        setLED(true);
        break;

    case LightMode::TIMED:
        setLED(true);
        break;
    }

    if (saveToEEPROM)
    {
        eepromWriteByte(
            EEPROM_MODE_ADDRESS,
            static_cast<uint8_t>(currentMode)
        );
    }

    printCurrentMode();
}

void nextMode()
{
    uint8_t next =
        (static_cast<uint8_t>(currentMode) + 1) % 4;

    setMode(static_cast<LightMode>(next));
}

void restoreSavedMode()
{
    uint8_t savedMode =
        eepromReadByte(EEPROM_MODE_ADDRESS);

    if (savedMode <= 3)
    {
        setMode(
            static_cast<LightMode>(savedMode),
            false
        );
    }
    else
    {
        setMode(LightMode::OFF, false);
    }
}

// ============================================================
// Controller State Machine
// ============================================================

void updateController()
{
    uint32_t now = getMilliseconds();

    switch (currentMode)
    {
    case LightMode::OFF:
    case LightMode::ON:
        break;

    case LightMode::BLINK:

        if ((uint32_t)(now - previousBlinkTime)
            >= BLINK_MS)
        {
            previousBlinkTime = now;
            setLED(!ledState);
        }

        break;

    case LightMode::TIMED:

        if ((uint32_t)(now - timedStartTime)
            >= TIMED_MS)
        {
            // Automatic expiration does not
            // overwrite the saved EEPROM mode.
            setMode(LightMode::OFF, false);
        }

        break;
    }
}

// ============================================================
// Button Handling
// ============================================================

void handleButton()
{
    uint32_t now = getMilliseconds();

    // Active-low button input
    bool rawButtonDown =
        (PIND & (1 << PIND2)) == 0;

    // Detect raw input changes
    if (rawButtonDown != lastRawButtonDown)
    {
        lastRawButtonDown = rawButtonDown;
        lastButtonChangeTime = now;
    }

    // Validate a stable button state
    if ((uint32_t)(now - lastButtonChangeTime)
        >= DEBOUNCE_MS)
    {
        if (rawButtonDown != stableButtonDown)
        {
            stableButtonDown = rawButtonDown;

            if (stableButtonDown)
            {
                // Validated press
                buttonPressStartTime = now;
                longPressHandled = false;
            }
            else
            {
                // Validated release
                if (!longPressHandled)
                {
                    nextMode();
                }
            }
        }
    }

    // Detect long press
    if (stableButtonDown && !longPressHandled)
    {
        if ((uint32_t)(now - buttonPressStartTime)
            >= LONG_PRESS_MS)
        {
            setMode(LightMode::OFF);

            longPressHandled = true;
        }
    }

    // Clear supplementary interrupt event flag.
    // Debouncing uses stable-state polling above.
    if (buttonEventPending)
    {
        uint8_t savedSREG = SREG;
        cli();
        buttonEventPending = false;
        SREG = savedSREG;
    }
}

// ============================================================
// Serial Command Processing
// ============================================================

void printHelp()
{
    usartPrintLine("");
    usartPrintLine("Available commands:");
    usartPrintLine("0 - OFF");
    usartPrintLine("1 - ON");
    usartPrintLine("2 - BLINK");
    usartPrintLine("3 - TIMED");
    usartPrintLine("n - Next mode");
    usartPrintLine("s - Show status");
    usartPrintLine("h - Help");
    usartPrintLine("");
}

void handleSerial()
{
    while (usartAvailable())
    {
        char command = usartRead();

        switch (command)
        {
        case '0':
            setMode(LightMode::OFF);
            break;

        case '1':
            setMode(LightMode::ON);
            break;

        case '2':
            setMode(LightMode::BLINK);
            break;

        case '3':
            setMode(LightMode::TIMED);
            break;

        case 'n':
        case 'N':
            nextMode();
            break;

        case 's':
        case 'S':
            printCurrentMode();
            break;

        case 'h':
        case 'H':
            printHelp();
            break;

        case '\r':
        case '\n':
            break;

        default:
            usartPrintLine(
                "Unknown command. Enter h for help."
            );
            break;
        }
    }
}

// ============================================================
// Main Entry Point - No Arduino Framework
// ============================================================

int main(void)
{
    cli();

    configureGPIO();
    configureUSART();
    configureTimer1();
    configureButtonInterrupt();

    // Initialize button state
    bool initialButtonDown =
        (PIND & (1 << PIND2)) == 0;

    lastRawButtonDown = initialButtonDown;
    stableButtonDown = initialButtonDown;
    lastButtonChangeTime = 0;
    buttonPressStartTime = 0;

    sei();

    usartPrintLine("");
    usartPrintLine("==========================");
    usartPrintLine("AVR Light Controller");
    usartPrintLine("Standalone AVR-GCC");
    usartPrintLine("==========================");

    restoreSavedMode();

    printHelp();

    while (1)
    {
        handleButton();
        updateController();
        handleSerial();
    }

    return 0;
}
