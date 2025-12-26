# Migration to ESPHome Native CC1101 Component

## Goal
Rewrite the Itho Fan component to use ESPHome's built-in CC1101 component instead of the external ITHO-Lib library. This provides better integration with ESPHome and removes the external dependency.

## Current Implementation (ITHO-Lib)
- Uses external library: `https://github.com/jodur/ITHO-Lib#NewLib`
- Custom interrupt handling
- Direct CC1101 register access via IthoCC1101 class

## Target Implementation (ESPHome CC1101)
- Uses ESPHome's native `cc1101` component
- Packet-based communication via triggers
- Standard ESPHome SPI device pattern

## Key Changes Required

### 1. Component Dependencies
**Before:**
```python
cg.add_library("https://github.com/jodur/ITHO-Lib#NewLib", None)
```

**After:**
```python
DEPENDENCIES = ["cc1101"]
```

### 2. Configuration Structure
**Before:**
```yaml
itho_fan:
  id: itho_hub
  device_id: "10,87,81"
  interrupt_pin: D1
```

**After:**
```yaml
spi:
  clk_pin: D5
  mosi_pin: D7
  miso_pin: D6

cc1101:
  id: cc1101_transceiver
  cs_pin: D8
  gdo0_pin: D1
  frequency: 868.299MHz
  modulation: ASK/OOK
  # Additional Itho-specific settings

itho_fan:
  id: itho_hub
  cc1101_id: cc1101_transceiver
  device_id: "10,87,81"
```

### 3. C++ Implementation Changes

#### Packet Reception
**Before (ITHO-Lib):**
```cpp
rf_.init();
rf_.initReceive();
if (rf_.checkForNewPacket()) {
  IthoCommand cmd = rf_.getLastCommand();
}
```

**After (ESPHome CC1101):**
```cpp
// Subscribe to CC1101 packet trigger
cc1101_->get_packet_trigger()->add_callback([this](std::vector<uint8_t> packet, float rssi, uint8_t lqi) {
  this->handle_packet(packet, rssi, lqi);
});
```

#### Packet Transmission
**Before (ITHO-Lib):**
```cpp
rf_.sendCommand(IthoLow);
```

**After (ESPHome CC1101):**
```cpp
std::vector<uint8_t> packet = build_itho_packet(IthoCommand::LOW);
cc1101_->transmit_packet(packet);
```

### 4. Itho Protocol Implementation

Need to implement Itho packet encoding/decoding:
- Packet structure (header, device ID, command, checksum)
- Command encoding (Low, Medium, High, Full, Timer1-3)
- Device ID handling
- CRC/checksum calculation

## Implementation Plan

### Phase 1: Research
- [x] Analyze ESPHome CC1101 component API
- [ ] Document Itho packet format from ITHO-Lib
- [ ] Identify required CC1101 register settings for Itho

### Phase 2: Core Changes
- [ ] Update `__init__.py` to use cc1101 dependency
- [ ] Modify `itho_fan.h` to reference CC1101Component
- [ ] Implement packet encoding/decoding functions
- [ ] Replace ITHO-Lib calls with ESPHome CC1101 API

### Phase 3: Testing
- [ ] Verify packet transmission
- [ ] Test packet reception
- [ ] Validate all fan speeds and timer modes
- [ ] Test remote tracking functionality

### Phase 4: Documentation
- [ ] Update README.md with new configuration
- [ ] Add CC1101 wiring diagram
- [ ] Migration guide for existing users

## Benefits of Native CC1101

1. **No External Dependencies** - Everything in ESPHome
2. **Better Integration** - Standard ESPHome patterns
3. **More Maintainable** - Community-supported component
4. **Enhanced Features** - RSSI, LQI, packet statistics
5. **Flexible Configuration** - All CC1101 settings exposed

## References

- ESPHome CC1101 Component: `/esphome/components/cc1101/`
- Home Assistant Forum: https://community.home-assistant.io/t/guide-controlling-itho-daalderop-fan-with-esp8266-and-cc1101/446808/43
- ITHO-Lib: https://github.com/jodur/ITHO-Lib

## Status

⚠️ **Work in Progress** - This branch is for development of the native CC1101 implementation.

For the current working version using ITHO-Lib, see the `copilot/restructure-itho-component` branch.
