# ESPHOME-ITHO

Pure YAML ESPHome configuration for Itho ventilation units using the native CC1101 component.

This is a **complete rewrite** that eliminates external C++ components in favor of a pure YAML implementation using ESPHome's built-in CC1101 support.

## ✨ Version 2.0 Features

### Pure YAML Implementation
- **No External Components**: Uses native ESPHome CC1101 component only
- **No Custom C++ Code**: Everything implemented in YAML with lambda functions
- **Easier Maintenance**: Standard ESPHome configuration patterns
- **Faster Compilation**: No external dependencies to build

### Core Functionality
- **Native Fan Component**: Full Home Assistant fan entity integration
- **4 Speed Levels**: Low, Medium, High, Full
- **Timer Presets**: 10, 20, and 30-minute timers with automatic countdown
- **Bi-directional RF Communication**: Both send and receive Itho commands
- **Device Whitelist**: Security feature to only accept commands from known remotes
- **Room/Remote Tracking**: Shows which room/remote triggered the command
- **Anti-Retransmit Protection**: Prevents infinite loops when receiving packets
- **JOIN/LEAVE Buttons**: Pair and unpair remotes directly from Home Assistant

### Technical Features
- **Verified Itho Protocol**: Correct 2-FSK modulation settings from community examples
- **Accurate Command Bytes**: Verified against original ITHO-Lib implementation
- **Decimal ID Format**: User-friendly decimal numbers (0-255) instead of hex
- **Array Configuration**: Clean, readable device ID setup
- **Preset Synchronization**: Timer status properly reflected in Home Assistant

## Hardware Requirements

- **ESP8266** (D1 Mini or compatible) - tested on D1 Mini
- **CC1101 RF Module** (868MHz version for EU)
- **Power Supply**: 5V for ESP8266

### Wiring (D1 Mini Example)

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

## Installation

### Quick Start

1. **Copy the configuration file**:
   ```bash
   wget https://raw.githubusercontent.com/jodur/ESPHOME-ITHO/feature/use-esphome-cc1101/itho.yaml
   ```

2. **Create `secrets.yaml`**:
   ```yaml
   wifi_ssid: "YourWiFiSSID"
   wifi_password: "YourWiFiPassword"
   ```

3. **Configure your Device ID** in `itho.yaml`:
   ```yaml
   substitutions:
     device_id: "10, 87, 81"  # CHANGE THIS to make your device unique!
   ```

4. **Add your Remote IDs** (around line 120 in `itho.yaml`):
   ```cpp
   RemoteID allowed_remotes[] = {
     {"51,40,61", "Badkamer"},   // Your remote 1 - CHANGE THESE!
     {"73,82,11", "Toilet"},     // Your remote 2
     {"10,87,81", "Keuken"}      // Your remote 3
   };
   ```

5. **Compile and flash**:
   ```bash
   esphome run itho.yaml
   esphome run itho.yaml
   ```

## Configuration

The configuration is a single YAML file (`itho.yaml`) with no external dependencies.

### Key Configuration Sections

#### 1. Device ID (Top of file)
```yaml
substitutions:
  device_id: "10, 87, 81"   # Your unique device ID (decimal 0-255 each)
```

Each ESP/CC1101 device must have a **unique** ID. Use decimal numbers (0-255).

#### 2. WiFi Credentials
Create a `secrets.yaml` file:
```yaml
wifi_ssid: "YourWiFiName"
wifi_password: "YourWiFiPassword"
```

#### 3. Remote Whitelist (in itho.yaml around line 120)
```cpp
RemoteID allowed_remotes[] = {
  {"51,40,61", "Badkamer"},   // Remote 1 - decimal format
  {"73,82,11", "Toilet"},     // Remote 2
  {"10,87,81", "Keuken"}      // Remote 3
};
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

### Pairing a New Remote

1. Click the **"Join/Pair Remote"** button in Home Assistant
2. Within 10 seconds, press any button on your Itho remote
3. The remote is now paired to your ventilation unit

To unpair: Use the **"Leave/Unpair Remote"** button instead.

## Migration from Version 1.x (C++ Component)

If you're upgrading from the old C++ custom component:

### What Changed

| Version 1.x (C++ Component) | Version 2.0 (Pure YAML) |
|---------------------------|------------------------|
| External C++ component | Native ESPHome CC1101 |
| `itho_fan` component | Standard `fan` template |
| Hex device IDs (0x0A) | Decimal IDs (10) |
| Component configuration | YAML lambda functions |
| Compile time: ~45s | Compile time: ~5-50s |

### Migration Steps

1. **Backup** your old configuration
2. **Replace** your entire configuration with `itho.yaml`
3. **Convert IDs** from hex to decimal:
   - Old: `device_id: "0x0A,0x57,0x51"`
   - New: `device_id: "10, 87, 81"`
4. **Update remote IDs** in the `allowed_remotes` array (around line 120)
5. **Create** `secrets.yaml` with WiFi credentials
6. **Flash** the new firmware

The fan entity name and functionality remain the same, so your Home Assistant automations should continue to work without changes.

## Changelog (v2.0)

### Major Changes
- ✅ **Pure YAML Implementation**: Removed all external C++ components
- ✅ **Native CC1101**: Uses ESPHome's built-in CC1101 component
- ✅ **Verified Protocol**: Applied correct Itho RF settings from community examples
- ✅ **Decimal Format**: User-friendly decimal IDs instead of hex
- ✅ **Array Configuration**: Cleaner device ID setup

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

### Command Bytes (from ITHO-Lib)

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

## Credits

Based on the original work from:
- [esphome_c1101](https://github.com/CoMPaTech/esphome_c1101)
- [ITHO-Lib](https://github.com/jodur/ITHO-Lib)
- [Community Guide](https://community.home-assistant.io/t/guide-controlling-itho-daalderop-fan-with-esp8266-and-cc1101/446808)

Special thanks to the Home Assistant community for verifying the CC1101 protocol settings.

## License

MIT License

