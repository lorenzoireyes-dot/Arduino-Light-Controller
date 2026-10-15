#include <Arduino.h>
#include <EEPROM.h>

// Hardware configuration
constexpr uint8_t LED_PIN = LED_BUILTIN;

// Timing configuration (milliseconds)
constexpr unsigned long BLINK_INTERVAL = 500;
constexpr unsigned long TIMED_DURATION = 10000;

constexpr int EEPROM_MODE_ADDRESS = 0;
constexpr uint8_t MAX_MODE_VALUE = 3;

constexpr uint8_t BUTTON_PIN = 2;

constexpr unsigned long DEBOUNCE_INTERVAL = 40;
constexpr unsigned long LONG_PRESS_DURATION = 1500;

// Shared between interrupt and main program
volatile bool buttonEventPending = false;

// Button processing state
bool stableButtonState = HIGH;
bool lastRawButtonState = HIGH;

unsigned long lastDebounceTime = 0;
unsigned long buttonPressStart = 0;

bool longPressHandled = false;

// Controller operating states
enum class LightMode : uint8_t {
  OFF,
  ON,
  BLINK,
  TIMED
};

// Current controller state
LightMode currentMode = LightMode::OFF;

// Timing variables
unsigned long previousBlinkTime = 0;
unsigned long timedStartTime = 0;

// Tracks the current LED output
bool ledState = false;

// Controls the physical LED
void setLED(bool state) {
  ledState = state;
  digitalWrite(LED_PIN, state ? HIGH : LOW);
}

const char* modeName(LightMode mode) {
  switch (mode) {
    case LightMode::OFF:   return "OFF";
    case LightMode::ON:    return "ON";
    case LightMode::BLINK: return "BLINK";
    case LightMode::TIMED: return "TIMED";
    default:               return "UNKNOWN";
  }
}

void saveMode(LightMode mode) {
  
  uint8_t value = static_cast<uint8_t>(mode);

  EEPROM.update(EEPROM_MODE_ADDRESS, value);
}

LightMode loadMode() {

  uint8_t storedValue = EEPROM.read(EEPROM_MODE_ADDRESS);

  if (storedValue <= MAX_MODE_VALUE) {
    return static_cast<LightMode>(storedValue);
  }

  // Invalid EEPROM data: default to OFF
  return LightMode::OFF;
}

// Handles state transitions
void setMode(LightMode newMode, bool persist = false) {
  
  // Update current state
  currentMode = newMode;

  if (persist) {
    saveMode(currentMode);
  }

  // Capture the time of transition
  unsigned long now = millis();

  // Reset timing variables
  previousBlinkTime = now;
  timedStartTime = now;

  // Initialize the new state
  switch (currentMode) {

    Serial.print(F("Mode changed to: "));
    Serial.println(modeName(currentMode));
    
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
}

// Executes the current state
void updateController() {

  unsigned long now = millis();

  switch (currentMode) {
    case LightMode::OFF:
      break;
    
    case LightMode::ON:
      break;

    case LightMode::BLINK:
      if (now - previousBlinkTime >= BLINK_INTERVAL) {

        previousBlinkTime = now;

        setLED(!ledState);
      }
      break;

    case LightMode::TIMED:
      
      if (now - timedStartTime >= TIMED_DURATION) {
        
        setMode(LightMode::OFF);
      }
      break;
  }
}

void nextMode() {
  
  switch (currentMode) {

    case LightMode::OFF:
      setMode(LightMode::ON, true);
      break;

    case LightMode::ON:
      setMode(LightMode::BLINK, true);
      break;

    case LightMode::BLINK:
      setMode(LightMode::TIMED, true);
      break;

    case LightMode::TIMED:
      setMode(LightMode::OFF, true);
      break;
  }
}

void handleSerial() {

  while (Serial.available() > 0) {
    char command = Serial.read();

    switch (command) {

      case '0':
        setMode(LightMode::OFF, true);
        break;

      case '1':
        setMode(LightMode::ON, true);
        break;

      case '2':
        setMode(LightMode::BLINK, true);
        break;

      case '3':
        setMode(LightMode::TIMED, true);
        break;

      case 's':
      case 'S':
        Serial.print(F("Current mode: "));
        Serial.println(modeName(currentMode));
        break;

      case 'h':
      case 'H':
        Serial.println(F("Commands:"));
        Serial.println(F("0 = OFF"));
        Serial.println(F("1 = ON"));
        Serial.println(F("2 = BLINK"));
        Serial.println(F("3 = TIMED"));
        Serial.println(F("n = NEXT MODE"));
        Serial.println(F("s = STATUS"));
        Serial.println(F("h = HELP"));
        break;

      default:
      // Ignore unsupported characters
      break;
    }
  }
}

void buttonISR() {
  buttonEventPending = true;
}

void handleButton() {

  unsigned long now = millis();

  // Atomically consume the interrupt event
  noInterrupts();
  bool eventPending = buttonEventPending;
  buttonEventPending = false;
  interrupts();

  // Read the physical input every iteration.
  bool rawState = digitalRead(BUTTON_PIN);

  if (rawState != lastRawButtonState) {
    lastDebounceTime = now;
    lastRawButtonState = rawState;
  }

  // Accept a change only after the input remains
  // stable for the debounce interval.
  if ((now - lastDebounceTime >= DEBOUNCE_INTERVAL) && (lastRawButtonState != stableButtonState)) {
    stableButtonState = rawState;

    if (stableButtonState == LOW) {

      // Button has been pressed.
      buttonPressStart = now;
      longPressHandled = false;
    
    } else {

      // Button has been released
      if (!longPressHandled) {
        nextMode();
      }
    }
  }

  // Detect a long press while button is held.
  if (stableButtonState == LOW && !longPressHandled && (now - buttonPressStart >= LONG_PRESS_DURATION)) {

    setMode(LightMode::OFF, true);
    longPressHandled = true;
  }

  // Event flag is retained for interrupt diagnostics.
  // It is not requred for debounce correctness
  (void)eventPending;
}

void setup() {
  pinMode(LED_PIN, OUTPUT);

  Serial.begin(9600);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  lastRawButtonState = digitalRead(BUTTON_PIN);
  stableButtonState = lastRawButtonState;
  lastDebounceTime = millis();

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, CHANGE);

  // Initialize controller
  LightMode restoredMode = loadMode();
  setMode(restoredMode);

  Serial.println(F("Programmable Light Controller"));
  Serial.println(F("System initialized."));
  Serial.println(F("Enter 'h' for available commands."));
}

void loop() {

  handleSerial();
  handleButton();
  // Execute current operating mode
  updateController();
}
