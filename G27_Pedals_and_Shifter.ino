/*
   G27 SHIFTER ONLY → ESP32 Thing Plus (Bluetooth LE Gamepad)
   ----------------------------------------------------------
   - Only H-pattern shifter + push-down reverse button
   - BLE exposes:
       * Button 1–6 = gears 1–6
       * Button 7   = Reverse (6th gear + reverse button)
*/

#include <Arduino.h>
#include <BleGamepad.h>

// ------------------------
// PIN ASSIGNMENTS
// ------------------------
// Analog axes from shifter pots
#define SHIFTER_X_PIN   32   // X-axis wiper (left–right)
#define SHIFTER_Y_PIN   33   // Y-axis wiper (forward–back)

// Single reverse button (push-down)
#define REVERSE_PIN     27   // digital input, to reverse microswitch

// ------------------------
// BLE Gamepad config
// ------------------------
BleGamepad bleGamepad("G27 Shifter ESP32", "DIY", 100);

// 1–6 = gears, 7 = reverse
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
// ANALOG THRESHOLDS
// (tune these to your hardware)
// ------------------------
// ADC is 0–4095. Start with these, then adjust
// after you log values in Serial Monitor.
int Y_TOP_THRESHOLD    = 2900;   // above this = top row (1,3,5)
int Y_BOTTOM_THRESHOLD = 1200;   // below this = bottom row (2,4,6)

// left / mid / right gates
int X_LEFT_THRESHOLD   = 1300;   // left gate (1/2)
int X_RIGHT_THRESHOLD  = 2800;   // right gate (5/6/R)

// ------------------------
// STATE
// ------------------------
int currentGear = 0;   // 0 = neutral, 1–6, 7 = reverse
int prevGear    = 0;

// ------------------------
// HELPERS
// ------------------------
int readAxisAveraged(int pin, int samples = 8) {
  long sum = 0;
  for (int i = 0; i < samples; ++i) {
    sum += analogRead(pin);
    delayMicroseconds(200);
  }
  return (int)(sum / samples);
}

// Determine gear from X/Y only (no reverse logic yet)
int detectBaseGear(int xRaw, int yRaw) {
  // Neutral if within mid Y-band
  if (yRaw <= Y_TOP_THRESHOLD && yRaw >= Y_BOTTOM_THRESHOLD) {
    return 0;
  }

  if (yRaw > Y_TOP_THRESHOLD) {
    // Top row: 1,3,5
    if (xRaw < X_LEFT_THRESHOLD) {
      return 1;   // 1st
    } else if (xRaw > X_RIGHT_THRESHOLD) {
      return 5;   // 5th
    } else {
      return 3;   // 3rd
    }
  } else if (yRaw < Y_BOTTOM_THRESHOLD) {
    // Bottom row: 2,4,6
    if (xRaw < X_LEFT_THRESHOLD) {
      return 2;   // 2nd
    } else if (xRaw > X_RIGHT_THRESHOLD) {
      return 6;   // 6th (R is a “mode” of this)
    } else {
      return 4;   // 4th
    }
  }

  return 0; // fallback neutral
}

void updateGamepadGear(int newGear) {
  if (!bleGamepad.isConnected()) return;

  // release previous
  if (prevGear > 0) {
    switch (prevGear) {
      case 1: bleGamepad.release(GP_GEAR_1); break;
      case 2: bleGamepad.release(GP_GEAR_2); break;
      case 3: bleGamepad.release(GP_GEAR_3); break;
      case 4: bleGamepad.release(GP_GEAR_4); break;
      case 5: bleGamepad.release(GP_GEAR_5); break;
      case 6: bleGamepad.release(GP_GEAR_6); break;
      case 7: bleGamepad.release(GP_REVERSE); break;
    }
  }

  // press new
  if (newGear > 0) {
    switch (newGear) {
      case 1: bleGamepad.press(GP_GEAR_1); break;
      case 2: bleGamepad.press(GP_GEAR_2); break;
      case 3: bleGamepad.press(GP_GEAR_3); break;
      case 4: bleGamepad.press(GP_GEAR_4); break;
      case 5: bleGamepad.press(GP_GEAR_5); break;
      case 6: bleGamepad.press(GP_GEAR_6); break;
      case 7: bleGamepad.press(GP_REVERSE); break;
    }
  }

  prevGear = newGear;
}

// ------------------------
// SETUP / LOOP
// ------------------------
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting G27 Shifter-only BLE HID...");

  pinMode(REVERSE_PIN, INPUT_PULLUP);  // assume switch to GND when pressed

  // configure BLE gamepad
  BleGamepadConfiguration config;
  config.setButtonCount(GAMEPAD_BUTTON_COUNT);
  config.setAutoReport(true);
  bleGamepad.begin(&config);

  Serial.println("Pair as: 'G27 Shifter ESP32'");
}

void loop() {
  // Read analogs
  int xRaw = readAxisAveraged(SHIFTER_X_PIN);
  int yRaw = readAxisAveraged(SHIFTER_Y_PIN);

  // Read reverse button (active LOW)
  bool reversePressed = (digitalRead(REVERSE_PIN) == LOW);

  int baseGear = detectBaseGear(xRaw, yRaw);
  int newGear  = baseGear;

  // If in 6th gate and reverse pushed, treat it as Reverse (7)
  if (baseGear == 6 && reversePressed) {
    newGear = 7;
  }

  if (newGear != currentGear) {
    currentGear = newGear;
    updateGamepadGear(currentGear);
  }

  // Debug
  static unsigned long lastPrint = 0;
  unsigned long now = millis();
  if (now - lastPrint > 250) {
    lastPrint = now;
    Serial.print("X=");
    Serial.print(xRaw);
    Serial.print("  Y=");
    Serial.print(yRaw);
    Serial.print("  base=");
    Serial.print(baseGear);
    Serial.print("  gear=");
    Serial.print(currentGear);
    Serial.print("  revBtn=");
    Serial.print(reversePressed ? "1" : "0");
    Serial.print("  BLE=");
    Serial.println(bleGamepad.isConnected() ? "ON" : "OFF");
  }

  delay(5);
}
