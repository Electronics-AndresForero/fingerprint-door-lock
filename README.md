# Fingerprint Door Lock

ESP32-based biometric access control system for a classroom closet. Unlocks a 12V solenoid lock when an enrolled fingerprint is recognized, with LED visual feedback.

---

## Demo

> Finger placed → green LED → solenoid clicks open → locks after 3 seconds

---

## Hardware

| Component            |                   Details                          |
|-----------           |----------------------------------------------------|
| Microcontroller      | ESP32 38-pin (breadboard)                          |
| Fingerprint Sensor   | Optical UART sensor (R307 compatible)              |
| Lock                 | 12V / 1A solenoid (cerradura solenoide)            |
| Relay                | SRD-05VDC-SL-C single channel                      |
| Buck Converter       | LM2596 with display (12V → 5V)                     |
| Power                | 12V 3A DC adapter + USB 5V for ESP32               |
| LEDs                 | Green (match) + Red (no match) with 220Ω resistors |

---

## Pin Map

| Signal                           | ESP32 GPIO   |  
|--------                          |--------------|
| Fingerprint TX (green wire)      | GPIO16 (RX2) |
| Fingerprint RX (white wire)      | GPIO17 (TX2) |
| Fingerprint Touch (yellow wire)  | GPIO23       |
| Fingerprint VIN (red wire)       | 3.3V         |
| Fingerprint GND (black wire)     | GND          |
| Fingerprint Touch VCC (blue wire)| 3.3V         |    
| Green LED (+)                    | GPIO22       |
| Red LED (+)                      | GPIO21       |
| Relay IN                         | GPIO27 |

---

## ⚡ Wiring Overview

```
12V Adapter
    │
    ├──► Barrel Jack Breakout
    │         ├── VCC ──► LM2596 IN+ ──► VOUT+ ──► Relay VCC (5V)
    │         └── GND ──► LM2596 IN−
    │
    └──► Relay COM ──► Solenoid (+)
                       Solenoid (−) ──► GND

ESP32 powered separately via USB 5V
```

> **Note:** ESP32 is powered via USB. The 12V circuit (solenoid + relay) is isolated. This avoids ground loop issues between the buck converter and ESP32 logic circuit.

---

##  Branches

| Branch | Description |
|--------|-------------|
| `main` | Phase 1 — fingerprint sensor test only, no solenoid |
| `phase2` | Phase 2 — full system with solenoid lock ✅ |

---

##  Getting Started

### Prerequisites
- [VS Code](https://code.visualstudio.com/)
- [PlatformIO extension](https://platformio.org/)

### Clone & Flash

```bash
git clone https://github.com/Electronics-AndresForero/fingerprint-door-lock.git
cd fingerprint-door-lock
git checkout phase2
```

Open in VS Code → PlatformIO will auto-install dependencies → click **Upload**.

> Hold the **BOOT** button on the ESP32 during upload if it fails to connect.

### Dependencies

Managed automatically by PlatformIO via `platformio.ini`:

```ini
lib_deps = adafruit/Adafruit Fingerprint Sensor Library@^2.1.0
```

---

## Serial Monitor Commands

Open Serial Monitor at **115200 baud** after flashing:

| Command | Action |
|---------|--------|
| `E` + Enter, then type ID (1-127) | Enroll a new fingerprint |
| `D` + Enter, then type ID | Delete a fingerprint |
| `L` | List all enrolled IDs |
| Place finger | Verify and unlock if matched |

---

## 🔁 How It Works

1. Sensor's touch pin detects finger presence (GPIO23)
2. UART communication captures and processes fingerprint image
3. Match found → GPIO27 triggers relay (active HIGH) for 3 seconds
4. Relay closes NO terminal → 12V flows to solenoid → door unlocks
5. Green LED on during unlock, red LED blinks on failed match
6. After 3 seconds relay releases → solenoid locks again

---

## ⚠️ Important Notes

- Use the **NO (Normally Open)** terminal on the relay — solenoid stays off by default
- Leave the **NC terminal empty**
- Solenoid gets hot if powered continuously — this design only pulses it briefly
- `RELAY_ACTIVE` is set to `HIGH` for this specific relay module — change to `LOW` if your relay behaves inversely

---

## Project Structure

```
FingerprintLock/
├── src/
│   └── main.cpp          # Main application code
├── platformio.ini         # PlatformIO config + dependencies
└── README.md
```

---

## Built With

- [Arduino Framework](https://www.arduino.cc/)
- [PlatformIO](https://platformio.org/)
- [Adafruit Fingerprint Sensor Library](https://github.com/adafruit/Adafruit-Fingerprint-Sensor-Library)

---

## Author

**Andres Forero**  
[github.com/Electronics-AndresForero](https://github.com/Electronics-AndresForero)
