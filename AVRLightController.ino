#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

// Timing configuration (milliseconds).
constexpr uint32_t DEBOUNCE_INTERVAL = 40;
constexpr uint32_t BLINK_INTERVAL = 500;
constexpr uint32_t TIMED_DURATION = 10000;
constexpr uint32_t LONG_PRESS_DURATION = 1500;
constexpr uint16_t EEPROM_MODE_ADDRESS = 0;

// Controller operating modes.
enum class LightMode : uint8_t
{
    OFF,
    ON,
    BLINK,
    TIMED
};

volatile uint32_t systemMilliseconds = 0;
volatile bool buttonEventPending = false;

LightMode currentMode = LightMode::OFF;

bool lastRawButtonDown = false;
bool stableButtonDown = false;
bool ledState = false;

uint32_t lastButtonChangeTime = 0;
uint32_t previousBlinkTime = 0;
uint32_t timedStartTime = 0;
uint32_t buttonPressStartTime = 0;
bool longPressHandled = false;

void printCurrentMode();

// Timer1 interrupt: 1 ms system tick.
ISR(TIMER1_COMPA_vect)
{
    systemMilliseconds++;
}

// INT0: records external button activity.
ISR(INT0_vect)
{
    buttonEventPending = true;
}

uint32_t getMilliseconds()
{
    uint8_t previousSREG = SREG;
    cli();

    uint32_t currentTime = systemMilliseconds;

    SREG = previousSREG;
    return currentTime;
}

void setLED(bool on)
{
    ledState = on;

    if (on)
    {
        PORTB |= (1 << PORTB5);
    }
    else
    {
        PORTB &= ~(1 << PORTB5);
    }
}

uint8_t eepromReadByte(uint16_t address)
{
    // Wait until any previous write has finished.
    while (EECR & (1 << EEPE))
    {
    }

    // Select the EEPROM address.
    EEAR = address;

    // Start the EEPROM read.
    EECR |= (1 << EERE);

    // Retrieve the byte.
    return EEDR;
}

void eepromWriteByte(uint16_t address, uint8_t data)
{
    // Wait for any previous EEPROM write to finish.
    while (EECR & (1 << EEPE))
    {
    }

    // Avoid unnecessary EEPROM wear.
    if (eepromReadByte(address) == data)
    {
        return;
    }

    // Set the target address and data.
    EEAR = address;
    EEDR = data;

    // Preserve the previous interrupt state.
    uint8_t previousSREG = SREG;
    cli();

    // Enable EEPROM programming.
    EECR |= (1 << EEMPE);

    // Start programming within four CPU cycles.
    EECR |= (1 << EEPE);

    // Restore the previous interrupt state.
    SREG = previousSREG;
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
        case LightMode::BLINK:
        case LightMode::TIMED:
            setLED(true);
            break;
    }

    // Save user-selected modes.
    if (saveToEEPROM)
    {
        eepromWriteByte(
            EEPROM_MODE_ADDRESS,
            static_cast<uint8_t>(currentMode)
        );
    }

    printCurrentMode();
}

void restoreSavedMode()
{
    uint8_t savedMode = eepromReadByte(EEPROM_MODE_ADDRESS);

    // Valid mode values are 0 through 3.
    if (savedMode <= static_cast<uint8_t>(LightMode::TIMED))
    {
        setMode(static_cast<LightMode>(savedMode), false);
    }
    else
    {
        // Invalid or uninitialized EEPROM.
        setMode(LightMode::OFF, false);
    }
}

void nextMode()
{
    switch (currentMode)
    {
        case LightMode::OFF:
            setMode(LightMode::ON);
            break;

        case LightMode::ON:
            setMode(LightMode::BLINK);
            break;

        case LightMode::BLINK:
            setMode(LightMode::TIMED);
            break;

        case LightMode::TIMED:
            setMode(LightMode::OFF);
            break;
    }
}

void updateController()
{
    uint32_t now = getMilliseconds();

    switch (currentMode)
    {
        case LightMode::OFF:
        case LightMode::ON:
            break;

        case LightMode::BLINK:
            if (now - previousBlinkTime >= BLINK_INTERVAL)
            {
                previousBlinkTime = now;
                setLED(!ledState);
            }
            break;

        case LightMode::TIMED:
            if (now - timedStartTime >= TIMED_DURATION)
            {
                setMode(LightMode::OFF, false);
            }
            break;
    }
}

void configureTimer1()
{
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    // 16 MHz / 64 / 1000 - 1 = 249.
    OCR1A = 249;

    // CTC mode with prescaler of 64.
    TCCR1B = (1 << WGM12) |
             (1 << CS11) |
             (1 << CS10);

    TIMSK1 |= (1 << OCIE1A);
}

void configureButtonInterrupt()
{
    // D2 / PD2 input with internal pull-up.
    DDRD &= ~(1 << DDD2);
    PORTD |= (1 << PORTD2);

    // INT0 falling-edge detection.
    EICRA = (EICRA & ~((1 << ISC01) | (1 << ISC00)))
          | (1 << ISC01);

    EIFR = (1 << INTF0);
    EIMSK |= (1 << INT0);
}


void handleButton()
{
    uint32_t now = getMilliseconds();
    bool rawButtonDown = !(PIND & (1 << PIND2));

    // Detect changes in the physical button signal.
    if (rawButtonDown != lastRawButtonDown)
    {
        lastRawButtonDown = rawButtonDown;
        lastButtonChangeTime = now;
    }

    // Accept a change only after 40 ms of stability.
    if ((now - lastButtonChangeTime >= DEBOUNCE_INTERVAL) &&
        (rawButtonDown != stableButtonDown))
    {
        stableButtonDown = rawButtonDown;

        if (stableButtonDown)
        {
            // A validated press has started.
            buttonPressStartTime = now;
            longPressHandled = false;
        }
        else
        {
            // A validated release has occurred.
            if (!longPressHandled)
            {
                nextMode();
            }
        }
    }

    // Handle a long press once, while the button is held.
    if (stableButtonDown &&
        !longPressHandled &&
        (now - buttonPressStartTime >= LONG_PRESS_DURATION))
    {
        setMode(LightMode::OFF);
        longPressHandled = true;
    }

    // Consume the supplementary interrupt flag.
    uint8_t previousSREG = SREG;
    cli();
    buttonEventPending = false;
    SREG = previousSREG;
}


void configureUSART()
{
    // 9600 baud at 16 MHz, normal speed.
    UBRR0H = 0;
    UBRR0L = 103;

    // Enable USART transmitter.
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);

    // 8 data bits, no parity, 1 stop bit.
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void usartTransmit(char data)
{
    // Wait until transmit buffer is available.
    while (!(UCSR0A & (1 << UDRE0)))
    {
    }

    UDR0 = data;
}

void usartPrint(const char* message)
{
    while (*message != '\0')
    {
        usartTransmit(*message);
        message++;
    }
}

void printCurrentMode()
{
    usartPrint("Current mode: ");

    switch (currentMode)
    {
        case LightMode::OFF:
            usartPrint("OFF");
            break;

        case LightMode::ON:
            usartPrint("ON");
            break;

        case LightMode::BLINK:
            usartPrint("BLINK");
            break;

        case LightMode::TIMED:
            usartPrint("TIMED");
            break;
    }

    usartPrint("\r\n");
}

bool usartAvailable()
{
    // RXC0 is set when unread received data is available.
    return (UCSR0A & (1 << RXC0)) != 0;
}

char usartRead()
{
    // Read the received character from USART0.
    return UDR0;
}

void handleSerial()
{
    if (!usartAvailable())
    {
        return;
    }

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
            usartPrint("\r\n");
            usartPrint("AVR Light Controller Commands\r\n");
            usartPrint("-----------------------------\r\n");
            usartPrint("0 - OFF\r\n");
            usartPrint("1 - ON\r\n");
            usartPrint("2 - BLINK\r\n");
            usartPrint("3 - TIMED (10 seconds)\r\n");
            usartPrint("n - Next mode\r\n");
            usartPrint("s - Current status\r\n");
            usartPrint("h - Help\r\n");
            break;

        // Ignore Serial Monitor line endings.
        case '\r':
        case '\n':
            break;

        default:
            usartPrint("Unknown command. Enter h for help.\r\n");
            break;
    }
}

void setup()
{
    // PB5 / D13: LED output.
    DDRB |= (1 << DDB5);
    setLED(false);

    configureTimer1();
    configureButtonInterrupt();

    lastRawButtonDown = !(PIND & (1 << PIND2));
    stableButtonDown = lastRawButtonDown;

    sei();

    configureUSART();

    usartPrint("\r\nAVR Light Controller initialized\r\n");

    restoreSavedMode();
}

void loop()
{
    handleButton();
    updateController();
    handleSerial();
}
