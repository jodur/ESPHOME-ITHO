# ESPHOME-ITHO

ESPHome custom component for Itho ventilation units using CC1101 RF communication.

This component provides **button-based control** with status sensors, designed for use with Home Assistant template fans to replicate the original master branch behavior where OFF state represents Low speed.

## Features

- **Button Controls**: Direct speed control (Low, Medium, High)
- **RF Communication**: Both send and receive commands via CC1101
- **Status Sensors**: String-based fan speed sensor and numeric speed sensor
- **Timer Support**: 10, 20, and 30-minute timer commands
- **Remote Tracking**: Identifies which remote/room triggered the state change
- **Device Name**: Customizable friendly name for the ESP device
- **Interrupt-driven**: Efficient packet reception
- **OTA Safe**: Automatically disables interrupts during updates
- **JOIN/LEAVE Support**: Pair and unpair remotes with the ventilation unit

## Hardware Requirements

- ESP8266 (D1 Mini or compatible) - tested and working
- CC1101 RF Module (868MHz version for EU)
- Power Supply: 5V for ESP8266
- Antenna: Connected to CC1101 ANT pin (critical for range!)

### Wiring (D1 Mini)

| CC1101 Pin | D1 Mini Pin | GPIO | Notes |
|------------|-------------|------|-------|
| VCC | 3.3V | - | **Important:** CC1101 requires 3.3V power! Do not connect to 5V. |
| GND | GND | - | |
| MOSI | D7 | GPIO13 | SPI Data Out |
| MISO | D6 | GPIO12 | SPI Data In |
| SCK | D5 | GPIO14 | SPI Clock |
| CSN | D8 | GPIO15 | SPI Chip Select |
| GDO0 | D1 | GPIO5 | Interrupt pin (packet received indicator) |
| GDO2 | - | - | Not connected |
| ANT | - | - | Connect antenna for proper RF range |

## Installation

### Option 1: Local Component (Recommended for Development)

1. Clone this repository or copy the `components/itho_fan` directory to your ESPHome config folder
2. Use the configuration example below:

```yaml
external_components:
  - source:
      type: local
      path: components
    components: [itho_fan]
```

### Option 2: External Git Repository

```yaml
external_components:
  - source: github://jodur/ESPHOME-ITHO
    components: [itho_fan]
```

## Configuration

### ESPHome Configuration

See [itho.yaml](itho.yaml) for complete working example.

**Key configuration:**

```yaml
external_components:
  - source:
      type: local
      path: components
    components: [itho_fan]

itho_fan:
  id: itho_hub
  device_id: "10,87,81"        # Your ESP device ID for sending commands
  device_name: "New ESP"       # Optional: Friendly name for Last Control Source
  interrupt_pin: D1              # GDO0 connected to GPIO5 (D1 on D1 Mini)
  remote_ids:
    - remote_id: "51,40,61"
      room_name: "Badkamer"

button:
  - platform: template
    name: "Fan Low"
    on_press:
      - lambda: id(itho_hub).send_command(1);
  # Add Medium, High, Join, Leave buttons similarly

sensor:
  - platform: template
    name: "Fan State"
    id: fan_state_numeric
    # Sensors are updated automatically by C++ component

text_sensor:
  - platform: template
    name: "Fan Speed"
    id: fanspeed
    # Registers with hub on boot, updated by C++ component
```

## Home Assistant Integration

Add this to your Home Assistant `configuration.yaml` to create a fan entity where OFF = Low speed (matching the original master branch behavior).

**Note:** Replace `fancontrol` in the example below with your ESPHome `device_name` (e.g., `new_esp`).

```yaml
template:
  - fan:
      - name: "Afzuiging badkamer"
        value_template: >
          {{ "off" if states('sensor.fancontrol_fan_speed') == 'Low' else "on" }}
        percentage_template: >
          {% set speedperc = {'Low': 0, 'Medium': 50, 'High': 100} %}
          {{ speedperc[states('sensor.fancontrol_fan_speed')] }}
        turn_on:
          service: button.press
          data:
            entity_id: button.fancontrol_fan_high
        turn_off:
          service: button.press
          data:
            entity_id: button.fancontrol_fan_low
        set_percentage:
          service: button.press
          data:
            entity_id: >
              {% set id_mapp = {0:'button.fancontrol_fan_low', 50:'button.fancontrol_fan_medium', 100:'button.fancontrol_fan_high'} %}
              {{ id_mapp[percentage] }}
        speed_count: 2
```

## Usage in Home Assistant

### ESPHome Entities

The component creates the following entities automatically:

- **Buttons**:
  - `button.{device_name}_fan_low`
  - `button.{device_name}_fan_medium`
  - `button.{device_name}_fan_high`
  - `button.{device_name}_join_remote` (for pairing)
  - `button.{device_name}_leave_remote` (for unpairing)

- **Sensors**:
  - `sensor.{device_name}_fan_state` (numeric: 1, 2, 3, 13, 23, 33)
  - `sensor.{device_name}_fan_speed` (text: "Low", "Medium", "High")
  - `sensor.{device_name}_fan_timer_remaining` (countdown timer like "9:45")
  - `sensor.{device_name}_last_control_source` (which remote/device last changed the state)

**Note:** Replace `{device_name}` with your ESPHome device name (e.g., `fancontrol`)

### Template Fan Entity

When using the Home Assistant template fan configuration:
- **OFF state** = Low speed (physical fan still running)
- **ON state** = Medium or High speed
- **Speed control**: 2 speeds (Medium at 50%, High at 100%)
- Turn OFF sets fan to Low speed
- Turn ON sets fan to High speed

### Example Automations

```yaml
# Set fan to medium speed
service: button.press
target:
  entity_id: button.fan_medium

# Set fan to low (via template fan OFF)
service: fan.turn_off
target:
  entity_id: fan.afzuiging_badkamer

# Set specific percentage (via template fan)
service: fan.set_percentage
target:
  entity_id: fan.afzuiging_badkamer
data:
  percentage: 50  # Medium speed
```

## Component Configuration Options

### `itho_fan` Platform

| Parameter | Required | Default | Description |
|-----------|----------|---------|-------------|
| `device_id` | Yes | - | ESP device ID for RF transmission (format: "X,X,X") |
| `device_name` | No | "ESPHome" | Friendly name shown in Last Control Source sensor |
| `interrupt_pin` | Yes | - | GPIO pin connected to CC1101 GDO0 (interrupt pin) |
| `remote_ids` | No | [] | List of remote IDs to recognize |

### Remote ID Configuration

Each remote in `remote_ids` requires:
- `remote_id`: RF ID in format "X,X,X"
- `room_name`: Friendly name shown in Last Control Source sensor

## Design Philosophy: Button-Based Control

This implementation uses **button entities** instead of a native ESPHome fan component for the following reasons:

1. **Master Branch Compatibility**: Replicates the original behavior where OFF = Low speed
2. **Home Assistant Flexibility**: Template fans provide complete control over UI state vs physical state
3. **Explicit Control**: Direct button presses are clear and unambiguous
4. **Simple Integration**: String-based sensor output works seamlessly with template fans

The ESPHome fan component doesn't support displaying OFF while the physical fan runs at low speed, which is the desired behavior from the original master branch implementation.

## Finding Remote IDs

To discover your remote IDs:

1. Set logger level to `DEBUG` or `VERBOSE`
2. Press a button on your remote
3. Check ESPHome logs for "Ignored device-id: X,X,X" or "Received ... from ..."
4. Add that ID to your `remote_ids` configuration with a friendly `room_name`

## Multiple ESP Setup

If running multiple ESPs in parallel (e.g., testing alongside the old master branch):

- Give each ESP a **different** `device_id` (e.g., "10,87,81" and "10,87,82")
- Add the other ESP's `device_id` to each ESP's `remote_ids` list
- This prevents the ITHO-Lib from filtering out packets from the other ESP
- Both ESPs will see each other's RF transmissions and external remote commands

Example for second ESP:
```yaml
itho_fan:
  device_id: "10,87,82"      # Different from first ESP
  device_name: "Test ESP"
  remote_ids:
    - remote_id: "10,87,81"  # First ESP's ID
      room_name: "Main ESP"
```

## Pairing with Your Itho Ventilation Unit

The JOIN and LEAVE buttons allow you to pair/unpair your ESP/CC1101 device with the ventilation unit.

### Pairing Procedure

The Itho ventilation unit must be in **pairing mode** to accept new devices:

1. **Enter pairing mode on ventilation unit:**
   - Turn OFF the power to the ventilation unit at the circuit breaker
   - Wait 10 seconds
   - Turn the power back ON
   - The unit is now in pairing mode for **30 seconds** after power-up

2. **Pair the ESP/CC1101:**
   - **Immediately** press the **"Join Remote"** button in Home Assistant
   - Wait for confirmation in the logs
   - Your ESP/CC1101 is now paired and can control the fan

3. **Test:**
   - Use Home Assistant to change fan speed
   - The ventilation unit should respond to commands

### Unpairing

To remove the ESP/CC1101 from the ventilation unit:
1. Follow the same power cycle procedure (step 1 above)
2. Press **"Leave Remote"** button instead
3. The ESP/CC1101 will no longer control the unit

**Note:** Your physical Itho remotes don't need to be re-paired after this. The ESP/CC1101 acts as an additional remote controller.

### RF Reliability

All commands are automatically sent **3 times**. This proven pattern:
- Significantly improves command reliability in noisy RF environments
- Is especially important for JOIN/LEAVE pairing commands
- Helps overcome temporary RF interference or obstructions

## Credits

Based on the original work from:
- [esphome_c1101](https://github.com/CoMPaTech/esphome_c1101)
- [ITHO-Lib](https://github.com/jodur/ITHO-Lib)
- [jodur/ESPEASY_Plugin_ITHO](https://github.com/jodur/ESPEASY_Plugin_ITHO) - Protocol verification

## License

MIT License

