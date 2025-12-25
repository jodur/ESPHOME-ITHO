# ESPHOME-ITHO

Modern ESPHome custom component for Itho ventilation units using CC1101 RF communication.

This component provides native ESPHome **Fan** integration with proper speed control and timer presets, eliminating the need for separate binary switches.

## Features

- **Native Fan Component**: Full integration with Home Assistant's fan entity
- **4 Speed Levels**: Low, Medium, High, Full
- **Timer Presets**: 10, 20, and 30-minute timers with countdown
- **RF Communication**: Both send and receive commands via CC1101
- **Remote Tracking**: Identifies which remote/room triggered the state change
- **Interrupt-driven**: Efficient packet reception
- **OTA Safe**: Automatically disables interrupts during updates

## Hardware Requirements

- ESP8266 (D1 Mini or compatible)
- CC1101 RF module
- Wiring: 
  - GDO0 → D1 (interrupt pin)
  - Other CC1101 pins according to SPI standard

## Installation

### Option 1: Local Component (Recommended for Development)

1. Clone this repository or copy the `components/itho_fan` directory to your ESPHome config folder
2. Use the configuration example below

### Option 2: External Git Repository

```yaml
external_components:
  - source: github://jodur/ESPHOME-ITHO
    components: [itho_fan]
```

## Configuration

```yaml
esphome:
  name: fancontrol
  on_boot:
    then: 
      - fan.turn_on: 
          id: itho_fan_component
          speed: 1

esp8266:
  board: d1_mini
  framework:
    version: recommended

external_components:
  - source:
      type: local
      path: components
    components: [itho_fan]

itho_fan:
  id: itho_hub
  device_id: "10,87,81"  # Your ESP device ID for sending commands
  interrupt_pin: D1
  remote_ids:
    - remote_id: "51,40,61"
      room_name: "Bathroom"
    - remote_id: "73,82,11"
      room_name: "Toilet"

fan:
  - platform: itho_fan
    id: itho_fan_component
    name: "Itho Ventilation"

text_sensor:
  - platform: template
    name: "Fan Timer Remaining"
    id: fan_timer
    icon: "mdi:timer"
    lambda: |-
      if (id(itho_hub).get_timer() > 0) {
        int minutes = id(itho_hub).get_timer() / 60;
        int seconds = id(itho_hub).get_timer() % 60;
        char buffer[20];
        sprintf(buffer, "%d:%02d", minutes, seconds);
        return {buffer};
      }
      return {"0"};
    update_interval: 1s
    
  - platform: template
    name: "Last Control Source"
    id: last_controller
    icon: "mdi:remote"
    lambda: |-
      return {id(itho_hub).get_last_id()};
    update_interval: 1s

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  ap:
    ssid: "FALLBACK AP"
    password: "jdsh348201"

captive_portal:

logger:
  level: DEBUG

api:

ota:
  on_begin:
    then:
      - lambda: |-
          ESP_LOGI("custom", "Disabling interrupts for OTA");
          detachInterrupt(D1);
```

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
```

## Migration from Old Structure

If you're upgrading from the old inline custom component:

1. **Remove** the old `itho.h` file
2. **Remove** `includes:` and `libraries:` from esphome section
3. **Add** the `external_components:` section
4. **Replace** all switch entities with the single fan entity
5. **Update** automations to use fan services instead of switches

### Old vs New Comparison

| Old Structure | New Structure |
|--------------|---------------|
| 7 separate switches | 1 fan entity with speed + presets |
| Manual state tracking | Automatic state management |
| Inline C++ code | Proper component structure |
| Basic sensors | Optional template sensors |

## Finding Remote IDs

To discover your remote IDs:

1. Set logger level to `VERBOSE`
2. Press a button on your remote
3. Check logs for "Ignored device-id: X,X,X"
4. Add that ID to your `remote_ids` configuration

## Credits

Based on the original work from:
- [esphome_c1101](https://github.com/CoMPaTech/esphome_c1101)
- [ITHO-Lib](https://github.com/jodur/ITHO-Lib)

## License

MIT License

