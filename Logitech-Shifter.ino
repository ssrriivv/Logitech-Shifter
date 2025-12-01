/*
   Logitech (G27?) SHIFTER ONLY → ESP32 Thing Plus (Bluetooth LE Gamepad)

   Hardware:
     - Shifter header: SCK, MISO, CS, X, Y, 3.3V, GND
     - ESP32 Thing Plus C

   Behavior:
     - X/Y → detect gears 1–6
     - Shift-register (SCK/MISO/CS) → reverse push-down
     - BLE Gamepad buttons:
         1–6 = gears 1–6
         7   = Reverse (6th + reverse pressed)
*/

#include <Arduino.h>
#include <BleGamepad.h>
#include "Filter.h"   // median filter

// ------------------------
// PIN ASSIGNMENTS (ESP32 Thing Plus)
// ------------------------
#define SHIFTER_CLOCK_PIN  13   // SCK from shifter
#define SHIFTER_DATA_PIN   12   // MISO from shifter
#define SHIFTER_MODE_PIN   14   // CS / !PL from shifter

#define SHIFTER_X_PIN      32   // X-axis ADC
#define SHIFTER_Y_PIN      33   // Y-axis ADC

#define BUTTON_REVERSE     1    // reverse push-down bit index

// ------------------------
// BLE CONFIG
// ------------------------
BleGamepad bleGamepad("Logitech Shifter", "DIY", 100);

enum GamepadButtons {
  GP_GEAR_1   = 1,
  GP_GEAR_2   = 2,
  GP_GEAR_3   = 3,
  GP_GEAR_4   = 4,
  GP_GEAR_5   = 5,
  GP_GEAR_6   = 6,
  GP_REVERSE  = 7
};

const int GAMEPAD_BUTTON_COUNT = 7;

// ------------------------
// ANALOG THRESHOLDS (BASED ON YOUR DATA)
// ------------------------
// Y: bottom ~0, neutral ~1575, top ~3200–3450
int Y_TOP_THRESHOLD    = 2400;   // above this = top row (1,3,5)
int Y_BOTTOM_THRESHOLD = 800;    // below this = bottom row (2,4,6)

// X: left ~610, neutral ~1450, right ~2170–2200
// lowered left threshold so 3/4 don't flip to 1/2 when you lean left
int X_LEFT_THRESHOLD   = 900;    // left gate (1/2) – moved further left
int X_RIGHT_THRESHOLD  = 1900;   // right gate (5/6/R)

// ------------------------
// FILTERS
// ------------------------
SignalFilter filterX;
SignalFilter filterY;

// ------------------------
// STATE
// ------------------------
int currentGear = 0;   // 0=N, 1–6, 7=R
int prevGear    = 0;

// ------------------------
// HELPERS
// ------------------------
static inline void settle() {
  delayMicroseconds(5);
}

void getButtonStates(int *ret) {
  digitalWrite(SHIFTER_MODE_PIN, LOW);   // latch
  settle();
  digitalWrite(SHIFTER_MODE_PIN, HIGH);  // shift mode

  for (int i = 0; i < 16; i++) {
    digitalWrite(SHIFTER_CLOCK_PIN, LOW);
    settle();
    ret[i] = digitalRead(SHIFTER_DATA_PIN);
    digitalWrite(SHIFTER_CLOCK_PIN, HIGH);
    settle();
  }
}

int readAxisFiltered(uint8_t pin, SignalFilter &filter) {
  uint16_t raw = analogRead(pin);
  uint16_t filtered = apply_filter(&filter, 9, raw);  // 9-sample median
  return (int)filtered;
}

int detectBaseGear(int x, int y) {
  // Neutral band in Y
  if (y <= Y_TOP_THRESHOLD && y >= Y_BOTTOM_THRESHOLD)
    return 0; // NEUTRAL

  if (y > Y_TOP_THRESHOLD) {
    // Top row: 1, 3, 5
    if (x < X_LEFT_THRESHOLD)       return 1;
    else if (x > X_RIGHT_THRESHOLD) return 5;
    else                            return 3;
  }

  if (y < Y_BOTTOM_THRESHOLD) {
    // Bottom row: 2, 4, 6
    if (x < X_LEFT_THRESHOLD)       return 2;
    else if (x > X_RIGHT_THRESHOLD) return 6;
    else                            return 4;
  }

  return 0;
}

void updateGamepadGear(int newGear) {
  if (!bleGamepad.isConnected()) return;

  // release previous
  if (prevGear > 0) {
    uint8_t btn = (prevGear == 7 ? GP_REVERSE : GP_GEAR_1 + (prevGear - 1));
    bleGamepad.release(btn);
  }

  // press new
  if (newGear > 0) {
    uint8_t btn = (newGear == 7 ? GP_REVERSE : GP_GEAR_1 + (newGear - 1));
    bleGamepad.press(btn);
  }

  prevGear = newGear;
}

// ------------------------
// SETUP
// ------------------------
void setup() {
  pinMode(SHIFTER_MODE_PIN, OUTPUT);
  pinMode(SHIFTER_CLOCK_PIN, OUTPUT);
  pinMode(SHIFTER_DATA_PIN, INPUT_PULLUP);

  digitalWrite(SHIFTER_MODE_PIN, HIGH);
  digitalWrite(SHIFTER_CLOCK_PIN, HIGH);

  BleGamepadConfiguration config;
  config.setButtonCount(GAMEPAD_BUTTON_COUNT);
  config.setAutoReport(true);
  bleGamepad.begin(&config);
}

// ------------------------
// LOOP
// ------------------------
void loop() {
  int buttonStates[16];
  getButtonStates(buttonStates);

  int  revRaw         = buttonStates[BUTTON_REVERSE];
  bool reversePressed = (revRaw == 1);   // 6 = 6, 6 + push-down = R

  int xFilt = readAxisFiltered(SHIFTER_X_PIN, filterX);
  int yFilt = readAxisFiltered(SHIFTER_Y_PIN, filterY);

  int baseGear = detectBaseGear(xFilt, yFilt);
  int newGear  = baseGear;

  if (baseGear == 6 && reversePressed)
    newGear = 7;

  if (newGear != currentGear) {
    currentGear = newGear;
    updateGamepadGear(currentGear);
  }

  // No delay → high polling rate
}
