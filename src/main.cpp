#include <Arduino.h>
#include <Adafruit_Fingerprint.h>

// ── Pin definitions ──────────────────────────────
#define PIN_FP_RX    16   // sensor TXD → ESP32 RX2
#define PIN_FP_TX    17   // sensor RXD → ESP32 TX2
#define PIN_TOUCH    23   // sensor YELLOW (touch detect)
#define PIN_LED_GREEN 22
#define PIN_LED_RED   21

// ── Fingerprint sensor on UART2 ──────────────────
HardwareSerial fpSerial(2);
Adafruit_Fingerprint finger(&fpSerial);

// ── Helpers ──────────────────────────────────────
void ledOff() {
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_RED, LOW);
}

void ledGreen(int ms = 1500) {
  ledOff(); digitalWrite(PIN_LED_GREEN, HIGH);
  delay(ms); ledOff();
}

void ledRed(int ms = 1000) {
  ledOff(); digitalWrite(PIN_LED_RED, HIGH);
  delay(ms); ledOff();
}

// ── Enroll a fingerprint ─────────────────────────
bool enrollFingerprint(uint8_t id) {
  Serial.printf("Enrolling ID #%d — place finger...\n", id);
  int result = -1;

  // Wait for finger
  while (result != FINGERPRINT_OK) {
    result = finger.getImage();
    if (result == FINGERPRINT_NOFINGER) { delay(100); continue; }
    if (result != FINGERPRINT_OK) { Serial.println("Image error"); return false; }
  }
  Serial.println("Image taken");
  if (finger.image2Tz(1) != FINGERPRINT_OK) { Serial.println("Conversion error"); return false; }

  Serial.println("Remove finger..."); delay(2000);
  while (finger.getImage() != FINGERPRINT_NOFINGER) delay(200);

  Serial.println("Place same finger again...");
  result = -1;
  while (result != FINGERPRINT_OK) {
    result = finger.getImage();
    if (result == FINGERPRINT_NOFINGER) { delay(100); continue; }
    if (result != FINGERPRINT_OK) { Serial.println("Image error"); return false; }
  }
  if (finger.image2Tz(2) != FINGERPRINT_OK) { Serial.println("Conversion error"); return false; }
  if (finger.createModel() != FINGERPRINT_OK) { Serial.println("Fingerprints didn't match!"); return false; }
  if (finger.storeModel(id) != FINGERPRINT_OK) { Serial.println("Store failed"); return false; }

  Serial.printf("✅ Fingerprint enrolled as ID #%d\n", id);
  return true;
}

// ── Verify a fingerprint ─────────────────────────
int verifyFingerprint() {
  if (finger.getImage() != FINGERPRINT_OK) return -1;
  if (finger.image2Tz() != FINGERPRINT_OK)  return -1;
  if (finger.fingerSearch() != FINGERPRINT_OK) return -1;
  return finger.fingerID;
}

// ── Setup ─────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  pinMode(PIN_TOUCH, INPUT_PULLDOWN);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  ledOff();

  fpSerial.begin(57600, SERIAL_8N1, PIN_FP_RX, PIN_FP_TX);
  finger.begin(57600);
  delay(500);

  if (finger.verifyPassword()) {
    Serial.println("✅ Fingerprint sensor found!");
    ledGreen(500); ledGreen(500);
  } else {
    Serial.println("❌ Sensor not found — check wiring!");
    while (1) { ledRed(300); delay(200); }
  }

  Serial.println("\nType 'E' + ENTER in Serial Monitor to enroll a finger");
  Serial.println("Then place finger to verify at any time\n");
}

// ── Loop ──────────────────────────────────────────
void loop() {
  // Check for enroll command from Serial Monitor
  if (Serial.available()) {
    char c = Serial.read();
      if (c == 'E' || c == 'e') {
        // Flush anything left in buffer (the \n from pressing Enter)
        while (Serial.available()) Serial.read();
        
        Serial.println("Enter ID to enroll (1-127):");
        
        // Wait properly for user to type the number
        while (!Serial.available()) delay(50);
        
        uint8_t id = (uint8_t)Serial.parseInt();
        
        // Flush again after reading
        while (Serial.available()) Serial.read();
        
        if (id < 1 || id > 127) { 
          Serial.println("Invalid ID — must be 1 to 127"); 
          return; 
        }
        enrollFingerprint(id) ? ledGreen() : ledRed();
      }
  }

  // Detect finger via touch pin (fast wake) or poll sensor
  bool touchDetected = digitalRead(PIN_TOUCH) == HIGH;
  if (touchDetected) {
    Serial.println("Finger detected — scanning...");
    int id = verifyFingerprint();
    if (id >= 0) {
      Serial.printf("✅ MATCH! ID #%d  Confidence: %d\n", id, finger.confidence);
      ledGreen();
    } else {
      Serial.println("❌ No match / scan error");
      ledRed();
    }
  }

  delay(100);
}