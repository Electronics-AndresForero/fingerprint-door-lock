#include <Arduino.h>
#include <Adafruit_Fingerprint.h>

// ── Pin definitions ──────────────────────────────
#define PIN_FP_RX     16
#define PIN_FP_TX     17
#define PIN_TOUCH     23
#define PIN_LED_GREEN 22
#define PIN_LED_RED   21
#define PIN_RELAY     27

// Most relay modules are ACTIVE LOW (pulls IN LOW to activate).
// If your relay activates when IN is HIGH, change to HIGH.
#define RELAY_ACTIVE   LOW
#define RELAY_INACTIVE HIGH

// How long the door stays unlocked (ms)
#define UNLOCK_DURATION_MS 3000

HardwareSerial fpSerial(2);
Adafruit_Fingerprint finger(&fpSerial);

// ── LED helpers ───────────────────────────────────
void ledOff() {
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_RED, LOW);
}

void ledGreen(int ms = 1500) { ledOff(); digitalWrite(PIN_LED_GREEN, HIGH); delay(ms); ledOff(); }
void ledRed(int ms = 800)   { ledOff(); digitalWrite(PIN_LED_RED, HIGH);   delay(ms); ledOff(); }
void ledBlink(int pin, int times, int ms = 150) {
  for (int i = 0; i < times; i++) {
    digitalWrite(pin, HIGH); delay(ms);
    digitalWrite(pin, LOW);  delay(ms);
  }
}

// ── Solenoid lock ─────────────────────────────────
void unlock() {
  Serial.println("🔓 UNLOCKED");
  digitalWrite(PIN_RELAY, RELAY_ACTIVE);
  digitalWrite(PIN_LED_GREEN, HIGH);
  delay(UNLOCK_DURATION_MS);
  digitalWrite(PIN_RELAY, RELAY_INACTIVE);
  ledOff();
  Serial.println("🔒 Locked");
}

// ── Enroll ────────────────────────────────────────
bool enrollFingerprint(uint8_t id) {
  ledBlink(PIN_LED_GREEN, 3);
  Serial.printf("Enrolling ID #%d — place finger...\n", id);
  int result = -1;

  while (result != FINGERPRINT_OK) {
    result = finger.getImage();
    if (result == FINGERPRINT_NOFINGER) { delay(100); continue; }
    if (result != FINGERPRINT_OK) { Serial.println("Image error"); ledRed(); return false; }
  }
  if (finger.image2Tz(1) != FINGERPRINT_OK) { Serial.println("Conversion error"); ledRed(); return false; }

  Serial.println("✔ Remove finger..."); delay(2000);
  while (finger.getImage() != FINGERPRINT_NOFINGER) delay(200);

  Serial.println("Place same finger again...");
  result = -1;
  while (result != FINGERPRINT_OK) {
    result = finger.getImage();
    if (result == FINGERPRINT_NOFINGER) { delay(100); continue; }
    if (result != FINGERPRINT_OK) { Serial.println("Image error"); ledRed(); return false; }
  }
  if (finger.image2Tz(2) != FINGERPRINT_OK) { Serial.println("Conversion error"); ledRed(); return false; }
  if (finger.createModel() != FINGERPRINT_OK) {
    Serial.println("❌ Fingerprints didn't match!");
    ledBlink(PIN_LED_RED, 3); return false;
  }
  if (finger.storeModel(id) != FINGERPRINT_OK) { Serial.println("Store failed"); ledRed(); return false; }

  Serial.printf("✅ Enrolled ID #%d\n", id);
  ledBlink(PIN_LED_GREEN, 3);
  return true;
}

// ── Verify ────────────────────────────────────────
int verifyFingerprint() {
  if (finger.getImage()     != FINGERPRINT_OK) return -1;
  if (finger.image2Tz()     != FINGERPRINT_OK) return -1;
  if (finger.fingerSearch() != FINGERPRINT_OK) return -1;
  return finger.fingerID;
}

// ── Print stored IDs to Serial ────────────────────
void listStoredIDs() {
  Serial.println("Stored fingerprints:");
  for (uint8_t i = 1; i <= 127; i++) {
    if (finger.loadModel(i) == FINGERPRINT_OK) {
      Serial.printf("  ID #%d\n", i);
    }
  }
}

// ── Setup ─────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  pinMode(PIN_TOUCH,     INPUT_PULLDOWN);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED,   OUTPUT);
  pinMode(PIN_RELAY,     OUTPUT);

  ledOff();
  digitalWrite(PIN_RELAY, RELAY_INACTIVE); // make sure locked on boot

  fpSerial.begin(57600, SERIAL_8N1, PIN_FP_RX, PIN_FP_TX);
  finger.begin(57600);
  delay(500);

  if (finger.verifyPassword()) {
    Serial.println("✅ Fingerprint sensor OK");
    Serial.printf("Capacity: %d prints\n", finger.capacity);
    ledBlink(PIN_LED_GREEN, 2, 200);
  } else {
    Serial.println("❌ Sensor not found — check wiring!");
    while (1) { ledBlink(PIN_LED_RED, 1, 300); delay(300); }
  }

  Serial.println("\nCommands (Serial Monitor 115200 baud):");
  Serial.println("  E  = enroll new fingerprint");
  Serial.println("  D  = delete a fingerprint ID");
  Serial.println("  L  = list enrolled IDs");
  Serial.println("  Place finger anytime to unlock\n");
}

// ── Loop ──────────────────────────────────────────
void loop() {
  // ── Serial commands ──
  if (Serial.available()) {
    char c = toupper(Serial.read());

    if (c == 'E') {
      // Flush \n left from pressing Enter
      while (Serial.available()) Serial.read();
      
      Serial.println("Enter ID (1-127):");
      
      while (!Serial.available()) delay(50);
      uint8_t id = (uint8_t)Serial.parseInt();
      
      // Flush again after reading
      while (Serial.available()) Serial.read();
      
      if (id >= 1 && id <= 127) enrollFingerprint(id);
      else Serial.println("Invalid ID — must be 1 to 127");
    }

    else if (c == 'D') {
      // Same fix for Delete
      while (Serial.available()) Serial.read();
      
      Serial.println("Enter ID to delete (1-127):");
      
      while (!Serial.available()) delay(50);
      uint8_t id = (uint8_t)Serial.parseInt();
      
      while (Serial.available()) Serial.read();
      
      if (finger.deleteModel(id) == FINGERPRINT_OK)
        Serial.printf("✅ Deleted ID #%d\n", id);
      else
        Serial.println("Delete failed");
    }

    else if (c == 'L') listStoredIDs();
  }

  // ── Finger detection via touch pin ──
  if (digitalRead(PIN_TOUCH) == HIGH) {
    Serial.println("Finger detected — scanning...");
    int id = verifyFingerprint();
    if (id >= 0) {
      Serial.printf("✅ ID #%d matched (confidence %d) → UNLOCKING\n", id, finger.confidence);
      unlock();
    } else {
      Serial.println("❌ Unknown fingerprint");
      ledBlink(PIN_LED_RED, 3);
    }
  }

  delay(100);
}