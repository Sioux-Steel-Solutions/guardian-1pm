# Guardian 1PM Firmware Specification

## Project Overview
Guardian 1PM is an IoT smart relay system with remote monitoring and control capabilities via MQTT. The device provides WiFi connectivity, web-based provisioning, and real-time relay control through a cloud-connected backend.

---

## Hardware Generations

### **Gen 1 - ESP8266EX Based** ✅ STABLE
- **Last Working Commit:** `57cb07ae31d5f8ca1f995603abad31207f472c59`
- **Commit Message:** "Fix Reconnect Interval"
- **Platform:** ESP8266EX (ESP-12E module)
- **Framework:** Arduino
- **Status:** Production-ready, archived for reference

### **Gen 2 - ESP32-C3 Based** 🚧 IN DEVELOPMENT
- **Platform:** ESP32-C3 (ESP32-C3-WROOM-02-N4 module)
- **Custom PCB:** Manufactured by JLCPCB (2-layer board)
- **Schematic:** SCH_Schematic1_2025-12-27.pdf
- **PCB Design:** EasyEDA V1.0
- **Hardware Features:**
  - Native USB programming (GPIO18/GPIO19)
  - 5V relay with optocoupler isolation
  - AC switch state detection with optocoupler
  - AC to DC power supply (HLK-5M05, 5V 3A)
  - 3.3V LDO regulator (TLV75733P, 1A)
  - Power isolation (USB vs external AC/DC)
  - Dual status LEDs (Red/Blue)
  - Snubber circuit for inductive load protection
  - Boot/Reset buttons
  - 10-pin debug header
- **Status:** Migration in progress

---

## Gen 2 Hardware Configuration (ESP32-C3)

### MCU Module
- **Part:** ESP32-C3-WROOM-02-N4 (Espressif)
- **LCSC:** C2934560
- **Flash:** 4MB
- **Features:** 2.4GHz WiFi, RISC-V single-core @ 160MHz
- **Native USB:** GPIO18 (D-), GPIO19 (D+)

### Pin Definitions ✅ **VERIFIED**
| GPIO | Function | Direction | Default State | Notes |
|------|----------|-----------|---------------|-------|
| **GPIO5** | **Relay Control** | Output | LOW | **RELAY_IN** - Drives PC817 optocoupler |
| **GPIO4** | **Switch Detection** | Input | Pulled HIGH (10kΩ) | **SW_DETECTION** - AC switch via H11AA1M opto |
| **GPIO1** | **Blue LED Control** | Output | HIGH (off) | **LED** - User status LED via 330Ω (U8) |
| GPIO18 | USB D- | I/O | - | Native USB programming |
| GPIO19 | USB D+ | I/O | - | Native USB programming |
| GPIO9 | BOOT Button | Input | Pulled HIGH | Active LOW, boot mode select |
| GPIO8 | User Button | Input | Pulled HIGH (10kΩ) | Available for custom use |
| EN | Reset | Input | Pulled HIGH (10kΩ) | Chip enable/reset |
| GPIO6 | Debug/Expansion | I/O | - | Available on header H1 pin 7 |
| GPIO7 | Debug/Expansion | I/O | - | Available on header H1 pin 5 |
| GPIO2 | Debug/Expansion | I/O | - | Available on header H1 pin 6 |
| GPIO3 | Debug/Expansion | I/O | - | Available on header H1 pin 8 |
| TXD0 | UART TX | Output | - | Serial debug on header H1 pin 3 |
| RXD0 | UART RX | Input | - | Serial debug on header H1 pin 4 |
| 3V3 | Power | - | 3.3V | Red LED (U7) always on - power indicator |

**Verified via PCB netlist analysis of ESP32-C3-WROOM-02 pad assignments.**

### Power Supply Chain
1. **AC Input** → DB302 terminal block (4-pin, 5mm pitch)
2. **Fuse** → SGT520-2A (2A @ 250V)
3. **AC-DC Converter** → HLK-5M05 (5V, 3A, isolated)
   - LCSC: C209907
   - Output: 5V @ up to 3A (15W)
4. **Power Isolation** → DMG2305UX-7 P-MOSFET (Q4)
   - Isolates USB power (VBUS) from external 5V
   - Reverse polarity protection via MS2A40LWS diode (D6)
5. **LDO Regulator** → TLV75733PDBVR (U6)
   - LCSC: C485517
   - Input: 5V, Output: 3.3V @ 1A
   - Powers ESP32-C3 module

### Relay Circuit
- **Relay:** SLA-5VDC-SL-C (Songle, K1)
  - LCSC: C688870
  - Coil: 5VDC
  - Contacts: SPDT (Single Pole Double Throw)
  - Switching: 10A @ 250VAC / 10A @ 30VDC
- **Isolation:** PC817X2CSP9F optocoupler (U10)
  - LCSC: C66405
  - Isolation voltage: 5000V RMS
  - Dual channel optocoupler
- **Driver:** BC817 NPN transistor (Q5)
  - LCSC: C2137
  - hFE: 100-250
- **Control Signal:** GPIO6 → RELAY_IN
- **Current Limiting:** 330Ω resistors (R14, R15)
- **Status LED:** KT-0603B (LED1) with 330Ω resistor (R16)
- **Flyback Protection:** MS2A40LWS diode (D7) across relay coil

### Switch Detection Circuit
- **Isolation:** H11AA1M optocoupler (U11)
  - LCSC: C416004
  - Isolation voltage: 4170V RMS
  - Detects AC switch state
- **Input:** Connected to AC line via switch (SW terminal)
- **Pull-up:** 10kΩ resistor (R23) to 3.3V
- **Output Signal:** SW_DETECTION → ESP32 GPIO (TBD)
- **Input Network:** 4× 100kΩ resistors (R19, R20, R21, R22) for voltage division

### Snubber Circuit (Load Protection)
- **Components:**
  - R17: 180pF @ 1kHz MOV (Metal Oxide Varistor)
  - C22: 100nF safety capacitor (275VAC X2 type)
  - R18: 100Ω resistor
- **Purpose:** Suppress voltage spikes from inductive loads
- **Location:** Connected across L_OUT (load output)

### Status LEDs
| LED | Part | Color | Control | Function |
|-----|------|-------|---------|----------|
| U7 | XL-1005SURC | Red | 3V3 rail | Power indicator (always on) |
| U8 | XL-1005UBC | Blue | "LED" signal | User/status control |

Both use 330Ω current limiting resistors (R7, R8).

### User Interface
- **SW1:** HX-B3U-1000P-1.6N tactile switch
  - Function: BOOT (GPIO9)
  - Size: 3×2.5×1.6mm SMD
  - LCSC: C492341221
- **SW2:** HX-B3U-1000P-1.6N tactile switch
  - Function: RESET (CHIP_PU/EN)
  - Size: 3×2.5×1.6mm SMD
  - Debouncing: 100nF capacitor (C4)

### Connectors
| Connector | Part | Purpose | Pins | Pitch |
|-----------|------|---------|------|-------|
| USB1 | TYPE-C 16PIN 2MD(073) | Power + Programming | 16 | USB-C |
| CN1 | DB302-5.0-4P-GN-S | AC Input + Switch | 4 | 5.0mm |
| H1 | PZ254V-11-10P | Debug Header | 10 | 2.54mm |

**H1 Debug Header Pinout:**
```
Pin 1:  CHIP_PU (Reset)
Pin 2:  GPIO9 (Boot)
Pin 3:  TXD0
Pin 4:  RXD0
Pin 5:  IO7
Pin 6:  IO2
Pin 7:  IO6
Pin 8:  IO3
Pin 9:  GND
Pin 10: 3V3
```

### USB Configuration
- **Native USB programming** via ESP32-C3's built-in USB Serial/JTAG controller
- **TVS Protection:** 3× LESD5Z5.0CT1G (U2, U3, U4)
  - LCSC: C5451650
  - Protects USB data lines and power
- **CC Pull-down:** 5.1kΩ resistors (R3, R4) on CC1/CC2 for USB-C spec compliance

### Capacitor Summary
| Value | Quantity | Voltage | Usage | Package |
|-------|----------|---------|-------|---------|
| 10µF | 1 | 10V | MCU decoupling | 0402 |
| 1µF | 4 | 16-50V | Power filtering | 0402/0805 |
| 100nF | 9 | 50-275V | Bypass/safety | 0402/0805/TH |
| 100µF | 1 | 35V | Bulk capacitor | 6.3mm dia |

### Safety Features
1. **Galvanic Isolation:**
   - AC-DC converter (HLK-5M05): Isolated output
   - Relay control optocoupler (PC817): 5000V isolation
   - Switch detection optocoupler (H11AA1M): 4170V isolation
2. **Overcurrent Protection:**
   - 2A fuse (F1) on AC input
3. **Overvoltage Protection:**
   - MOV (R9, R17) on AC input and load output
   - USB TVS diodes (U2, U3, U4)
4. **Reverse Polarity:**
   - Schottky diode (D6) protects against reverse voltage
5. **Inductive Load Protection:**
   - Snubber circuit (R17, C22, R18)
   - Flyback diode (D7) across relay coil

---

## Gen 1 Hardware Configuration

### Pin Definitions
| Pin | Function | Direction | Default State | Notes |
|-----|----------|-----------|---------------|-------|
| GPIO0 | Status LED | Output | HIGH (OFF) | Active LOW, indicates WiFi/connection status |
| GPIO15 | Relay Control | Output | LOW (OFF) | HIGH = ON, LOW = OFF |

### Pin Behaviors
- **GPIO0 (LED):**
  - `LOW` - Connection attempt in progress or AP mode active
  - `HIGH` - WiFi connected successfully

- **GPIO15 (Relay):**
  - `LOW` - Relay circuit open (default/safe state)
  - `HIGH` - Relay circuit closed (energized)

---

## Firmware Architecture

### Core Libraries
```ini
ESP8266WebServer    # HTTP API server
PubSubClient        # MQTT client
ArduinoJson         # JSON serialization
LittleFS            # File system (declared but not actively used)
Bounce2             # Debouncing (declared but not implemented)
Ticker              # Timer utilities
EEPROM              # Persistent storage (512 bytes)
```

### Memory Layout (EEPROM)
```c
struct Config {
  char ssid[32];          // WiFi SSID
  char password[64];      // WiFi password
  char uuid[37];          // User UUID (from backend)
  char deviceId[37];      // Device UUID (from backend)
  uint32_t checksum;      // Data integrity check
}
// Total: ~172 bytes of 512 available
```

---

## Control Flow

### Startup Sequence
1. **Serial Initialization** - 115200 baud
2. **EEPROM Init** - Load 512-byte config space
3. **GPIO Setup:**
   - Set GPIO0 (LED) as OUTPUT, HIGH
   - Set GPIO15 (Relay) as OUTPUT, LOW
4. **Credential Check:**
   - Read and validate EEPROM config (checksum verification)
   - If valid → proceed to WiFi connection
   - If invalid → start Access Point mode
5. **WiFi Connection:**
   - Attempt connection for 15 seconds
   - On success → set LED HIGH, proceed
   - On failure → start AP mode `"Guardian_GMS_Relay"`
6. **Time Synchronization** - NTP sync with `pool.ntp.org`
7. **Web Server Start** - Port 80, API routes active
8. **MQTT Connection** - Connect to broker, subscribe to control topic

### Main Loop Operations
| Task | Interval | Condition | Action |
|------|----------|-----------|--------|
| HTTP Request Handling | Every loop | Always | `server.handleClient()` |
| WiFi Reconnection | 30s | WiFi disconnected | Attempt reconnect, re-sync NTP |
| MQTT Reconnection | 10s | MQTT disconnected | Reconnect and resubscribe |
| Heartbeat Publish | 60s | MQTT connected | Publish relay state |
| Loop Delay | - | Always | 100ms delay |

---

## MQTT Communication

### Topic Structure
```
Subscribe: /toDevice/{userId}/{deviceId}
Publish:   /toDaemon/{userId}/{deviceId}
```

### Inbound Commands (Subscribe Topic)
```json
// Relay Control
{
  "power": true  // or false
}

// System Commands
{
  "command": "restart"      // Reboot ESP
}
{
  "command": "clearEEPROM"  // Factory reset
}
```

### Outbound Messages (Publish Topic)
```json
{
  "relay_update": true,
  "update_type": "init" | "state_change" | "heartbeat",
  "relay_state": true,        // Current relay state
  "device_type": "relay",
  "status": "online",
  "user_id": "uuid-string",
  "device_id": "uuid-string",
  "timestamp": 1234567890      // millis() since boot
}
```

### Update Types
- `init` - Published on MQTT connection (device boot/reconnect)
- `state_change` - Published when relay state changes
- `heartbeat` - Published every 60 seconds

---

## Web API Endpoints

### Provisioning API
| Endpoint | Method | Purpose | Request Body | Response |
|----------|--------|---------|--------------|----------|
| `/` | GET | Health check | - | `{"status":"success"}` |
| `/scanWifi` | GET | Scan available networks | - | JSON array of networks |
| `/connect` | POST | Provision device | JSON with credentials | Success/failure status |

### LED Control (Debug)
| Endpoint | Method | Action |
|----------|--------|--------|
| `/led/on` | GET | Turn LED on (GPIO0 LOW) |
| `/led/off` | GET | Turn LED off (GPIO0 HIGH) |

### Provisioning Request Format
```json
POST /connect
{
  "ssid": "MyNetwork",
  "password": "wifi-password",
  "userId": "user-uuid-here",
  "deviceId": "device-uuid-here",
  "brokerAddress": "mqtt.example.com"  // Currently unused
}
```

### WiFi Scan Response Format
```json
{
  "data": [
    {
      "ssid": "NetworkName",
      "signal_level": -45,
      "channel": 6,
      "security": "WPA2/PSK"
    }
  ]
}
```

---

## State Machine

### Operational States
1. **Unconfigured** - No EEPROM credentials
   - LED: ON (GPIO0 LOW)
   - AP Mode: Active (`Guardian_GMS_Relay`)
   - Web Server: Accepting provisioning requests

2. **Configured - Connecting**
   - LED: ON (GPIO0 LOW)
   - WiFi: Attempting connection (15s timeout)

3. **Connected - Online**
   - LED: OFF (GPIO0 HIGH)
   - WiFi: Connected
   - MQTT: Connected and subscribed
   - Heartbeat: Active

4. **Connected - MQTT Offline**
   - LED: OFF (GPIO0 HIGH)
   - WiFi: Connected
   - MQTT: Reconnecting every 10s

5. **Disconnected - Reconnecting**
   - WiFi: Attempting reconnect every 30s
   - MQTT: Paused until WiFi returns

---

## Relay Control Logic

### Initialization
```cpp
pinMode(RELAY_CONTROL, OUTPUT);
digitalWrite(RELAY_CONTROL, LOW);  // Safe default: OFF
relayState = false;
```

### State Management
- **Turn ON:** `digitalWrite(RELAY_CONTROL, HIGH)` + `relayState = true`
- **Turn OFF:** `digitalWrite(RELAY_CONTROL, LOW)` + `relayState = false`
- **Toggle:** Invert `relayState` and apply to GPIO15

### State Persistence
- Currently NOT persisted to EEPROM
- Relay always starts in OFF state after power cycle/reboot

---

## Security Considerations

### Current Implementation
- ✅ WiFi credentials stored in EEPROM with checksum validation
- ✅ CORS headers on all API endpoints (`Access-Control-Allow-Origin: *`)
- ❌ No encryption on MQTT connection (plaintext)
- ❌ No authentication on web API (open in AP mode)
- ❌ No TLS/SSL on web server
- ❌ AP mode has no password (`WiFi.softAP("Guardian_GMS_Relay", "")`)

### Known Limitations
- MQTT broker credentials hardcoded in `secrets.h` (not in repo)
- No OTA update mechanism
- No device authentication beyond UUID matching

---

## Error Handling

### WiFi Connection Failures
- Timeout after 15 seconds
- Falls back to AP mode
- Retries every 30 seconds in main loop

### MQTT Connection Failures
- Retries every 10 seconds
- Continues operating web server during MQTT outage
- Publishes state update on successful reconnection

### EEPROM Corruption
- Checksum validation on every read
- Factory reset available via `/connect` endpoint or MQTT command

---

## Known Issues & Technical Debt

1. **Duplicate Response Sends** - `api.cpp:172` sends response twice in `/connect` handler
2. **Topic String Lifetime** - MQTT topic C-strings created from temporary String objects (potential dangling pointers)
3. **Unused Libraries** - LittleFS and Bounce2 declared but not implemented
4. **No Switch Monitoring** - Hardware capability exists but not implemented
5. **Hardcoded Magic Numbers** - Intervals and timeouts not in config constants
6. **No Watchdog Timer** - Device can hang indefinitely
7. **Memory Leaks** - String allocations in tight loops (e.g., `getUserId()` called frequently)

---

## Build Configuration

### PlatformIO Settings
```ini
[env:esp12e]
platform = espressif8266
board = esp12e
framework = arduino
monitor_speed = 115200
upload_port = COM7
```

### Required Secrets File
Create `src/utils/secrets.h`:
```cpp
#ifndef SECRETS_H
#define SECRETS_H

#define MQTT_SERVER "your-mqtt-broker.com"
#define MQTT_PORT 1883

#endif
```

---

## Gen 1 → Gen 2 Migration Map

### GPIO Mapping Changes
| Function | Gen 1 (ESP8266) | Gen 2 (ESP32-C3) | Notes |
|----------|-----------------|------------------|-------|
| Status LED | GPIO0 | TBD (3V3 rail for red) | Gen 2 has 2 LEDs |
| Relay Control | GPIO15 | GPIO6 | Different pin, same concept |
| Switch Detection | Not implemented | TBD | **NEW in Gen 2** |
| Boot Button | - | GPIO9 | Physical button added |
| Reset | External reset | EN pin | Physical button added |
| USB Data | - | GPIO18/19 | Native USB (new) |
| Debug UART | TXD0/RXD0 (GPIO1/3) | TXD0/RXD0 | Header-based |

### Platform Changes
| Aspect | Gen 1 | Gen 2 | Impact |
|--------|-------|-------|--------|
| MCU Core | Xtensa (80MHz) | RISC-V (160MHz) | Different instruction set |
| Flash | Not specified | 4MB | More storage available |
| Programming | UART (Serial) | Native USB | No USB-UART bridge needed |
| Power Supply | External 5V assumed | Integrated AC-DC | Self-contained unit |
| Isolation | None | Triple optocoupler | Much safer design |
| WiFi Library | ESP8266WiFi | WiFi (ESP32) | API changes needed |
| Web Server | ESP8266WebServer | WebServer (ESP32) | API mostly compatible |

### New Hardware Features (Gen 2)
1. **AC Switch Detection Circuit**
   - Optically isolated AC switch monitoring
   - Allows firmware to detect manual switch override
   - Enables smart automation based on physical switch state

2. **Native USB Programming**
   - No external USB-UART chip needed
   - Faster development/debug cycle
   - USB serial available for logging

3. **Integrated Power Supply**
   - Direct AC mains input (90-264VAC)
   - On-board AC-DC converter
   - Regulated 3.3V for MCU

4. **Physical Buttons**
   - BOOT button for firmware upload mode
   - RESET button for manual restart
   - Improves development experience

5. **Enhanced Safety**
   - 5000V optocoupler isolation on relay
   - 4170V optocoupler isolation on switch detection
   - Fuse protection on AC input
   - Snubber circuit for inductive loads

### Code Migration Checklist
- [ ] Update platformio.ini for ESP32-C3
- [ ] Change `ESP8266WiFi.h` → `WiFi.h`
- [ ] Change `ESP8266WebServer.h` → `WebServer.h`
- [ ] Update GPIO pin definitions
- [ ] Implement switch detection interrupt handler
- [ ] Add switch state to MQTT messages
- [ ] Update LED control logic (2 LEDs instead of 1)
- [ ] Test native USB serial output
- [ ] Verify EEPROM → Preferences/NVS migration
- [ ] Update watchdog timer for ESP32
- [ ] Test WiFi reconnection on new platform
- [ ] Validate MQTT performance

---

## Next Steps for Gen 2

### Hardware Migration Tasks
- [ ] Update platform to `espressif32` (ESP32-C3)
- [ ] Update board definition
- [ ] Map GPIO pins to new PCB layout
- [ ] Add switch monitoring inputs
- [ ] Verify relay driver compatibility

### Firmware Enhancements
- [ ] Implement switch state monitoring
- [ ] Add debouncing for physical switches
- [ ] Publish switch state changes via MQTT
- [ ] Add OTA update capability
- [ ] Implement watchdog timer
- [ ] Add MQTT over TLS
- [ ] Fix duplicate response bug
- [ ] Refactor MQTT topic string handling
- [ ] Add relay state persistence to EEPROM
- [ ] Implement proper secrets management

---

## Testing Checklist

### Gen 1 Validation
- [x] Fresh device boots into AP mode
- [x] WiFi provisioning via `/connect` endpoint
- [x] WiFi credentials persist across reboot
- [x] MQTT connection established
- [x] Relay control via MQTT `{"power": true/false}`
- [x] Heartbeat messages published every 60s
- [x] WiFi auto-reconnect after network outage
- [x] MQTT auto-reconnect after broker restart
- [x] Factory reset via `{"command": "clearEEPROM"}`
- [x] Device reboot via `{"command": "restart"}`

### Gen 2 Requirements
- [ ] All Gen 1 functionality preserved
- [ ] Switch state monitoring functional
- [ ] Switch state published to MQTT
- [ ] New PCB pin mappings verified
- [ ] Custom hardware BOM components validated

---

**Document Version:** 1.0
**Last Updated:** 2026-01-30
**Author:** Kaleb Tringale
**Gen 1 Stable Commit:** `57cb07ae31d5f8ca1f995603abad31207f472c59`
