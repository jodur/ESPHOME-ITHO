# ESPHOME-ITHO

Pure YAML ESPHome configuration for Itho CVE ECO ventilation units using the native CC1101 component.

This implementation uses **100% verified Itho protocol** with custom Manchester-like encoding, identical to the proven ESPEasy plugin implementation.

## ✨ Features

### Pure YAML Implementation
- **No External Components**: Uses native ESPHome CC1101 component only
- **No Custom C++ Code**: Everything implemented in YAML with lambda functions
- **Easier Maintenance**: Standard ESPHome configuration patterns
- **Faster Compilation**: No external dependencies to build

### Core Functionality
- **Verified Itho Protocol**: Custom Manchester-like encoding (NOT standard Manchester!)
- **100% ESPEASY Compatible**: Encoding/decoding identical to proven jodur/ESPEASY_Plugin_ITHO
- **Bi-directional RF Communication**: Both send and receive Itho commands with proper encoding
- **Native Fan Component**: Full Home Assistant fan entity integration
- **3 Speed Levels**: Low, Medium, High (4th speed mapped to High)
- **Timer Presets**: 10, 20, and 30-minute timers with automatic countdown
- **Device Whitelist**: Security feature to only accept commands from known remotes
- **Room/Remote Tracking**: Shows which room/remote triggered the command
- **Anti-Retransmit Protection**: Prevents infinite loops when receiving packets
- **Rollcode Counter**: Replay attack prevention with persistent counter
- **JOIN/LEAVE Support**: Pair and unpair with ventilation unit

### Technical Features
- **Custom Manchester-like Encoding**: Extracts even bits (0,2,4,6) from 10-bit groups
- **24-Byte Packet Structure**: 63 raw bytes → 24 decoded bytes with STARTBYTE=2
- **Device ID Transmission**: Unique ID in bytes 1-3 of every packet
- **Verified Command Bytes**: 100% identical to ESPEasy/ITHO-Lib implementations
- **Accurate RF Settings**: 2-FSK, 868.2999MHz, 38.383kBaud, sync word 0xB3 0x2A
- **Decimal ID Format**: User-friendly decimal numbers (0-255) instead of hex
- **Checksum Validation**: 256-sum algorithm on bytes 1-10
- **Preset Synchronization**: Timer status properly reflected in Home Assistant

## Hardware Requirements

- **ESP8266** (D1 Mini or compatible) - tested and working
- **CC1101 RF Module** (868MHz version for EU)
- **Power Supply**: 5V for ESP8266
- **Antenna**: Connected to CC1101 ANT pin (critical for range!)

### Tested Hardware
- Wemos D1 Mini (ESP8266)
- CC1101 868MHz module  
- Itho CVE ECO ventilation unit

### Wiring (D1 Mini)

| CC1101 Pin | D1 Mini Pin | ESP8266 GPIO |
|------------|-------------|--------------|
| VCC        | 3.3V        | -            |
| GND        | GND         | -            |
| MOSI       | D7          | GPIO13       |
| MISO       | D6          | GPIO12       |
| SCK        | D5          | GPIO14       |
| CSN        | D8          | GPIO15       |
| GDO0       | D1          | GPIO5        |

**Important**: CC1101 requires 3.3V power! Do not connect to 5V.

## Installation & Configuration

### Quick Start

1. **Download the configuration file**:
   ```bash
   wget https://raw.githubusercontent.com/jodur/ESPHOME-ITHO/feature/use-esphome-cc1101/itho.yaml
   ```

2. **Create `secrets.yaml`** in the same directory:
   ```yaml
   wifi_ssid: "YourWiFiSSID"
   wifi_password: "YourWiFiPassword"
   ```

3. **Configure device ID** (top of `itho.yaml`):
   ```yaml
   substitutions:
     device_id: "10, 87, 81"  # CHANGE THIS to make your device unique!
     controller_name: "Home Assistant"  # Displayed as control source
   ```
   
   **Important**: Each ESP/CC1101 device must have a **unique** ID. Use decimal numbers (0-255 each).

4. **Configure remote whitelist** (around line 120 in `itho.yaml`):
   ```cpp
   RemoteID allowed_remotes[] = {
     {"51,40,61", "Badkamer"},   // Your remote 1 - CHANGE THESE!
     {"73,82,11", "Toilet"},     // Your remote 2
     {"10,87,81", "Keuken"}      // Your remote 3
   };
   ```
   
   Only commands from these device IDs will be accepted (security feature).

5. **Compile and flash**:
   ```bash
   esphome run itho.yaml
   ```

### Finding Your Remote IDs

1. Enable debug logging (already enabled in itho.yaml)
2. Press a button on your Itho remote
3. Check ESPHome logs for: `"Device ID: X,Y,Z"`
4. Add that ID to the `allowed_remotes` array in **decimal format**

Example log output:
```
[D][itho:108]: Device ID: 51,40,61 (hex: 33,28,3D)
```
Use the decimal values `51,40,61` in your configuration.

## What's Included

The `itho.yaml` configuration provides:

- **Fan Entity**: `fan.itho_ventilation`
  - 4 speed levels (Low, Medium, High, Full)
  - 3 timer presets (10min, 20min, 30min)
  
- **Text Sensors**:
  - `text_sensor.fan_timer_remaining` - Shows countdown (e.g., "3:45")
  - `text_sensor.last_control_source` - Shows which room/remote was used

- **Buttons**:
  - `button.join_pair_remote` - Pair new remotes to your Itho unit
  - `button.leave_unpair_remote` - Unpair/remove remotes

All components appear automatically in Home Assistant via the API integration.

## CC1101 Protocol Settings

The configuration uses verified Itho protocol settings:
- **Frequency**: 868.2999 MHz (EU ISM band)
- **Modulation**: 2-FSK (NOT ASK/OOK!)
- **Symbol Rate**: 38383 Baud
- **Filter Bandwidth**: 203 kHz
- **FSK Deviation**: 50 kHz
- **Sync Mode**: 16/16 with sync words 0xB3, 0x2A
- **Packet Length**: 63 bytes (fixed)
- **CRC**: Disabled
- **Whitening**: Disabled

These settings are based on verified community examples and match the original Itho protocol exactly.

## Usage in Home Assistant

The fan appears as a native fan entity with:
- **On/Off** control
- **4-speed slider**: Low (1), Medium (2), High (3), Full (4)
- **Preset modes**: "Timer 10min", "Timer 20min", "Timer 30min"

### Example Automations

```yaml
# Set fan to medium speed
service: fan.set_percentage
target:
  entity_id: fan.itho_ventilation
data:
  percentage: 50  # Medium speed

# Activate 20-minute timer
service: fan.set_preset_mode
target:
  entity_id: fan.itho_ventilation
data:
  preset_mode: "Timer 20min"

# Turn on low speed
service: fan.turn_on
target:
  entity_id: fan.itho_ventilation
data:
  speed: 1
```

### Pairing the CC1101/ESP with Your Itho Ventilation Unit

**Important:** The JOIN/LEAVE buttons pair/unpair **your CC1101 device** (ESP) with the ventilation unit, not remotes!

#### Pairing Procedure

The Itho ventilation unit must be in **pairing mode** to accept new devices:

1. **Enter pairing mode on ventilation unit:**
   - Turn OFF the power to the ventilation unit at the circuit breaker
   - Wait 10 seconds
   - Turn the power back ON
   - The unit is now in pairing mode for **30 seconds** after power-up

2. **Pair the CC1101/ESP:**
   - **Immediately** click the **"Join/Pair"** button in Home Assistant
   - Wait for confirmation in the logs
   - Your ESP/CC1101 is now paired and can control the fan

3. **Test:**
   - Use Home Assistant to change fan speed
   - The ventilation unit should respond to commands

#### Unpairing

To remove the CC1101/ESP from the ventilation unit:
1. Follow the same power cycle procedure (step 1 above)
2. Click **"Leave/Unpair"** button instead
3. The CC1101/ESP will no longer control the unit

**Note:** Your physical Itho remotes don't need to be re-paired after this. The CC1101/ESP acts as an additional remote controller.

### Pairing Physical Itho Remotes (Original Remotes)

Physical Itho remotes are paired directly with the ventilation unit using their own pairing procedure (consult your Itho remote manual). The CC1101/ESP will receive and display commands from these paired remotes if their IDs are in the `allowed_remotes` whitelist.

## Migration from Version 1.x (C++ Component)

If you're upgrading from the old C++ custom component:

### What Changed

| Version 1.x (C++ Component) | Version 2.0 (Pure YAML) |
|---------------------------|------------------------|
| External C++ component | Native ESPHome CC1101 |
| `itho_fan` component | Standard `fan` template |
| Lambda in `on_boot` | `on_packet` trigger |
| `Idlist` array | `allowed_remotes` array |
| `rf.setDeviceID()` | `substitutions` |
| Compile time: ~45s | Compile time: ~5-50s |

### Migration Steps

1. **Backup** your old configuration
2. **Replace** your entire configuration with `itho.yaml`
3. **Move device ID** to substitutions at the top:
   - Old: `rf.setDeviceID(10,87,81);` in lambda
   - New: `device_id: "10, 87, 81"` in substitutions
4. **Update remote IDs** in the `allowed_remotes` array (around line 120):
   - Old: `Idlist[0]={"51,40,61","Badkamer"};`
   - New: `{"51,40,61", "Badkamer"},`
5. **Create** `secrets.yaml` with WiFi credentials
6. **Flash** the new firmware

The fan entity name and functionality remain the same, so your Home Assistant automations should continue to work without changes.

## Changelog

### v2.0 - Custom Manchester-like Encoding (2026-01-03)

#### Protocol Implementation
- ✅ **Custom Encoding/Decoding**: Implemented Itho's proprietary Manchester-like encoding
  - Decoding: Extracts even bits (0,2,4,6) from 10-bit groups
  - Encoding: Inserts 1-0 pattern after every 8 data bits
  - 63 raw bytes ↔ 24 decoded bytes conversion
- ✅ **100% ESPEASY Compatible**: Verified byte-for-byte against jodur/ESPEASY_Plugin_ITHO
- ✅ **Device ID Mechanism**: Unique 3-byte ID transmitted in every packet (bytes 1-3)
- ✅ **Rollcode Counter**: Replay attack prevention with persistent storage

#### Command Verification  
- ✅ **All Commands Verified**: Compared against three reference implementations:
  - jodur/ESPEASY_Plugin_ITHO (proven working in production)
  - arjenhiemstra/IthoEcoFanRFT (original protocol documentation)
  - RonaldHiemstra/esphome-itho (ESPHome port)
- ✅ **Speed Commands**: LOW (0x02), MEDIUM (0x03), HIGH (0x04)
- ✅ **Timer Commands**: 10min (0x0A), 20min (0x14), 30min (0x1E)
- ✅ **JOIN/LEAVE**: Pairing commands with proper encoding

#### Features
- ✅ **Pure YAML Implementation**: No external C++ components required
- ✅ **Native CC1101**: Uses ESPHome 2025.12+ built-in component
- ✅ **Bi-directional**: Send and receive with identical encoding
- ✅ **Device Whitelist**: Security filtering for known remotes
- ✅ **Room Tracking**: Display which remote triggered command
- ✅ **Timer Countdown**: Automatic fan state management
- ✅ **Anti-Retransmit**: Prevents packet loops

#### Technical
- ✅ **Checksum Algorithm**: 256 - sum(bytes 1-10)
- ✅ **Packet Structure**: 24-byte decoded format documented
- ✅ **RF Settings**: 2-FSK, 868.2999MHz, 38.383kBaud verified
- ✅ **Compilation**: Successful on ESP8266 (39.7% RAM, 39.6% Flash)

### v1.x - C++ Component Implementation

### Major Changes
- ✅ **Pure YAML Implementation**: Removed all external C++ components
- ✅ **Native CC1101**: Uses ESPHome's built-in CC1101 component
- ✅ **Verified Protocol**: Applied correct Itho RF settings from community examples
- ✅ **Simplified Configuration**: Substitutions instead of lambda initialization
- ✅ **Cleaner Structure**: `on_packet` trigger instead of `on_boot` lambda

### New Features
- ✅ **Device Whitelist**: Security feature - only accept known remotes
- ✅ **Room Tracking**: Shows which room/remote sent the command
- ✅ **Anti-Retransmit**: Prevents infinite loops when receiving packets
- ✅ **JOIN/LEAVE Buttons**: Pair/unpair remotes from Home Assistant
- ✅ **Preset Sync**: Timer status properly reflected in HA
- ✅ **Accurate Commands**: All command bytes verified against ITHO-Lib

### Bug Fixes
- ✅ Fixed LOW/HIGH macro conflicts with Arduino
- ✅ Corrected all command bytes to match original protocol
- ✅ Fixed preset mode synchronization issues
- ✅ Proper timer countdown implementation

## Credits
## Troubleshooting

### Remote Commands Not Working

1. Check device is receiving packets: Look for `"Received packet"` in logs
2. Verify remote ID is in whitelist: Check for `"Ignored packet from unknown device"`
3. Convert hex to decimal if needed: `0x33` = 51 decimal
4. Check CC1101 wiring, especially GDO0 pin

### Fan Not Responding to Commands

1. Verify CC1101 is transmitting: Look for `"Sent speed X command"` in logs
2. Check device ID is configured correctly
3. Ensure CC1101 has good antenna connection
4. Try pairing again using JOIN button

### Timer Not Counting Down

1. Check `fan_timer` global variable in logs
2. Verify interval is running (should update every second)
3. Check preset mode is set correctly in Home Assistant

### Compilation Issues

- ESPHome version must be 2023.x or newer for native CC1101 support
- ESP8266 board recommended: `d1_mini`
- If errors about CC1101: Update ESPHome to latest version

## Technical Details

### Itho Custom "Manchester-like" Encoding

**Important**: Itho does NOT use standard IEEE 802.3 Manchester encoding!

The protocol uses a custom encoding scheme:
- **Encoding**: Inserts `1-0` pattern after every 8 data bits
- **Decoding**: Extracts even bits (0, 2, 4, 6) from 10-bit groups
- **Packet Size**: 63 raw bytes → 24 decoded bytes
- **STARTBYTE**: Skip first 2 bytes after sync word

This implementation is **100% identical** to:
- ESPEasy IthoCC1101.cpp plugin
- arjenhiemstra/IthoEcoFanRFT
- RonaldHiemstra/esphome-itho

### Packet Structure (24 Decoded Bytes)

```
Byte 0:      Rollcode (low byte)
Bytes 1-3:   Device ID (3 bytes) - YOUR UNIQUE ID
Byte 4:      Rollcode (high byte)  
Bytes 5-10:  Command bytes (6 bytes)
Byte 11:     Checksum (256 - sum of bytes 1-10)
Bytes 12-23: Padding/unused
```

**Device ID Mechanism:**
- Every packet includes the sender's device ID in bytes 1-3
- Ventilation unit learns device IDs during JOIN command
- Unit only accepts commands from paired device IDs
- Receiver filters packets using `allowed_remotes` whitelist

### Command Bytes (Verified Against ESPEASY)

Speed commands (byte 6 = 0xF1):
```
LOW:    0x22 0xF1 0x03 0x00 0x02 0x04
MEDIUM: 0x22 0xF1 0x03 0x00 0x03 0x04
HIGH:   0x22 0xF1 0x03 0x00 0x04 0x04
```

Timer commands (byte 6 = 0xF3):
```
TIMER 10min: 0x22 0xF3 0x03 0x00 0x00 0x0A
TIMER 20min: 0x22 0xF3 0x03 0x00 0x00 0x14
TIMER 30min: 0x22 0xF3 0x03 0x00 0x00 0x1E
```

Pairing commands:
```
JOIN:  0x1F 0xC9 0x0C 0x00 0x1F 0xC9
LEAVE: 0x1F 0xC9 0x06 0x00 0x1F 0xC9
```

All command bytes verified byte-for-byte against jodur/ESPEASY_Plugin_ITHO.

### CC1101 Register Settings

```yaml
Frequency:       868.2999 MHz (EU ISM band)
Modulation:      2-FSK (NOT ASK/OOK!)
Symbol Rate:     38383 Baud (MDMCFG4=0x5A, MDMCFG3=0x83)
Filter BW:       203 kHz (MDMCFG4=0x5A)
FSK Deviation:   50 kHz (DEVIATN=0x50)
Sync Mode:       16/16 bits
Sync Word:       0xB3 0x2A (SYNC1=179, SYNC0=42)
Packet Length:   63 bytes (fixed, after sync)
CRC:             Disabled
Whitening:       Disabled
```

These settings match the proven ESPEasy implementation exactly.

## Technical Details

### Legacy Command Reference (v1.x)

All commands use this packet structure:
```
[0x16, 0xA4, DeviceID_1, DeviceID_2, DeviceID_3, CommandBytes...]
```

Speed commands (0xF1):
- LOW: `{0x22, 0xF1, 0x03, 0x00, 0x02, 0x04}`
- MEDIUM: `{0x22, 0xF1, 0x03, 0x00, 0x03, 0x04}`
- HIGH: `{0x22, 0xF1, 0x03, 0x00, 0x04, 0x04}`
- FULL: `{0x22, 0xF1, 0x03, 0x00, 0x04, 0x04}` (same as HIGH)

Timer commands (0xF3):
- TIMER 10min: `{0x22, 0xF3, 0x03, 0x00, 0x00, 0x0A}`
- TIMER 20min: `{0x22, 0xF3, 0x03, 0x00, 0x00, 0x14}`
- TIMER 30min: `{0x22, 0xF3, 0x03, 0x00, 0x00, 0x1E}`

Pairing commands:
- JOIN: `{0x1F, 0xC9, 0x0C, 0x00, 0x22, 0xF1}`
- LEAVE: `{0x1F, 0xC9, 0x06, 0x00, 0x1F, 0xC9}`

### Packet Format

Itho packets (after CC1101 removes sync bytes):
```
Byte 0:    Counter (rolling)
Byte 1-3:  Device ID (3 bytes)
Byte 4-9:  Command bytes (6 bytes)
```

### Interrupt Handling vs Polling

**Important technical difference from Version 1.x:**

- **Version 1.x (C++ Component)**: Used hardware interrupts on GDO0 pin for immediate packet detection
- **Version 2.0 (Native CC1101)**: Uses **polling** in the `loop()` method

The native ESPHome CC1101 component doesn't use hardware interrupts. Instead, it polls the GDO0 pin in the `loop()` function:

```cpp
void CC1101Component::loop() {
  if (this->gdo0_pin_ == nullptr || !this->gdo0_pin_->digital_read()) {
    return;  // No packet received
  }
  // Read packet from FIFO...
}
```

**How it works:**
1. GDO0 is configured to assert HIGH when packet is received (register: `GDO0_CFG = 0x01`)
2. ESPHome `loop()` runs continuously, checking GDO0 pin state
3. When HIGH, packet data is read from RX FIFO
4. RSSI and LQI are read from status registers
5. `on_packet` trigger fires with packet data

**Performance:** This polling approach is fast enough for Itho remote commands (which are infrequent). The main loop in ESPHome typically runs every few milliseconds, providing adequate responsiveness for user interactions.

**Advantages of polling method:**
- Simpler code, no interrupt context switching
- No race conditions between ISR and main code
- Easier debugging
- More portable across platforms

**When polling might be insufficient:**
- High-frequency packet bursts (>100 packets/sec)
- Time-critical protocols requiring <1ms response
- Battery-powered scenarios where deep sleep is needed

For Itho ventilation control, the polling method works perfectly fine.

## Credits

Based on and verified against:
- [jodur/ESPEASY_Plugin_ITHO](https://github.com/jodur/ESPEASY_Plugin_ITHO) - Authoritative reference implementation
- [arjenhiemstra/IthoEcoFanRFT](https://github.com/arjenhiemstra/IthoEcoFanRFT) - Original protocol documentation
- [RonaldHiemstra/esphome-itho](https://github.com/RonaldHiemstra/esphome-itho) - ESPHome implementation
- [CoMPaTech/esphome_c1101](https://github.com/CoMPaTech/esphome_c1101) - Original C++ component (v1.x)
- [Community Guide](https://community.home-assistant.io/t/guide-controlling-itho-daalderop-fan-with-esp8266-and-cc1101/446808) - CC1101 settings verification

Special thanks to the Home Assistant community for protocol verification and testing.

## License

MIT License

