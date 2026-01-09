#include "itho_fan.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"

namespace esphome {
namespace itho_fan {

static const char *const TAG = "itho_fan";

// Timer constants
static const int TIME_10MIN = 10 * 60;
static const int TIME_20MIN = 20 * 60;
static const int TIME_30MIN = 30 * 60;

// Global reference for interrupt handler
static IthoFanHub *global_itho_hub = nullptr;

void IRAM_ATTR itho_interrupt_handler(IthoFanHub *arg) {
  if (arg != nullptr) {
    arg->has_packet_ = true;
  }
}

// IthoFanHub Implementation

void IthoFanHub::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Itho Fan Hub...");
  
  global_itho_hub = this;
  
  rf_.init();
  
  if (interrupt_pin_ != nullptr) {
    interrupt_pin_->setup();
    interrupt_pin_->pin_mode(gpio::FLAG_INPUT);
    interrupt_pin_->attach_interrupt(itho_interrupt_handler, this, gpio::INTERRUPT_FALLING_EDGE);
  }
  
  rf_.initReceive();
  
  // Set device ID for sending
  uint8_t id[3];
  sscanf(device_id_.c_str(), "%hhu,%hhu,%hhu", &id[0], &id[1], &id[2]);
  rf_.setDeviceID(id[0], id[1], id[2]);
  
  init_complete_ = true;
  last_id_ = "System";
  
  ESP_LOGCONFIG(TAG, "Itho Fan Hub setup complete");
}

void IthoFanHub::loop() {
  if (has_packet_) {
    has_packet_ = false;
    process_packet();
  }
  
  // Handle timer countdown
  static uint32_t last_update = 0;
  uint32_t now = millis();
  
  if (now - last_update >= 1000) {
    last_update = now;
    
    if (state_ >= 10 && timer_ > 0) {
      timer_--;
      
      // Match master branch: publish all sensors when timer is running
      publish_sensors();
      
      if (timer_ <= 0) {
        set_state(1, 0, last_id_);
      }
      
      if (fan_ != nullptr) {
        fan_->update_state();
      }
    }
  }
}

void IthoFanHub::dump_config() {
  ESP_LOGCONFIG(TAG, "Itho Fan Hub:");
  ESP_LOGCONFIG(TAG, "  Device ID: %s", device_id_.c_str());
  if (interrupt_pin_ != nullptr) {
    LOG_PIN("  Interrupt Pin: ", interrupt_pin_);
  }
  ESP_LOGCONFIG(TAG, "  Registered Remotes: %d", remotes_.size());
  for (const auto &remote : remotes_) {
    ESP_LOGCONFIG(TAG, "    - ID: %s, Room: %s", remote.id.c_str(), remote.room_name.c_str());
  }
}

void IthoFanHub::set_device_id(uint8_t id1, uint8_t id2, uint8_t id3) {
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%d,%d,%d", id1, id2, id3);
  device_id_ = buffer;
}

void IthoFanHub::add_remote_id(const std::string &id, const std::string &room_name) {
  remotes_.push_back({id, room_name});
}

void IthoFanHub::send_command(uint8_t command) {
  noInterrupts();
  
  switch (command) {
    case 1:
      rf_.sendCommand(IthoLow);
      set_state(1, 0, device_name_);
      break;
    case 2:
      rf_.sendCommand(IthoMedium);
      set_state(2, 0, device_name_);
      break;
    case 3:
      rf_.sendCommand(IthoHigh);
      set_state(3, 0, device_name_);
      break;
    case 4:
      rf_.sendCommand(IthoFull);
      set_state(4, 0, device_name_);
      break;
    case 13:
      rf_.sendCommand(IthoTimer1);
      set_state(13, TIME_10MIN, device_name_);
      break;
    case 23:
      rf_.sendCommand(IthoTimer2);
      set_state(23, TIME_20MIN, device_name_);
      break;
    case 33:
      rf_.sendCommand(IthoTimer3);
      set_state(33, TIME_30MIN, device_name_);
      break;
  }
  
  interrupts();
  rf_.initReceive();
  
  if (fan_ != nullptr) {
    fan_->update_state();
  }
}

void IthoFanHub::send_join() {
  noInterrupts();
  rf_.sendCommand(IthoJoin);
  interrupts();
  rf_.initReceive();
  ESP_LOGI(TAG, "Join command sent");
}

void IthoFanHub::send_leave() {
  noInterrupts();
  rf_.sendCommand(IthoLeave);
  interrupts();
  rf_.initReceive();
  ESP_LOGI(TAG, "Leave command sent");
}

void IthoFanHub::set_state(int state, int timer, const std::string &id) {
  bool state_changed = (state_ != state);
  
  state_ = state;
  timer_ = timer;
  last_id_ = id;
  
  // Match master branch: publish when state changes
  if (state_changed) {
    publish_sensors();
  }
}

void IthoFanHub::publish_sensors() {
  // Publish all sensors together (matches master branch behavior)
  if (state_sensor_ != nullptr) {
    state_sensor_->publish_state(state_);
  }
  
  if (speed_sensor_ != nullptr) {
    std::string speed_text;
    if (state_ == 1) speed_text = "Low";
    else if (state_ == 2) speed_text = "Medium";
    else if (state_ == 3 || state_ == 13 || state_ == 23 || state_ == 33) speed_text = "High";
    else speed_text = "Unknown";
    speed_sensor_->publish_state(speed_text);
  }
  
  if (timer_sensor_ != nullptr) {
    if (timer_ > 0) {
      int minutes = timer_ / 60;
      int seconds = timer_ % 60;
      char buffer[20];
      sprintf(buffer, "%d:%02d", minutes, seconds);
      timer_sensor_->publish_state(buffer);
    } else {
      timer_sensor_->publish_state("0");
    }
  }
  
  if (controller_sensor_ != nullptr) {
    controller_sensor_->publish_state(last_id_);
  }
}

void IthoFanHub::on_homeassistant_connected() {
  // Publish current state to Home Assistant (don't change fan state)
  publish_sensors();
}

int IthoFanHub::get_remote_index(const std::string &id) {
  for (size_t i = 0; i < remotes_.size(); i++) {
    if (remotes_[i].id == id) {
      return i;
    }
  }
  return -1;
}

void IthoFanHub::process_packet() {
  noInterrupts();
  
  if (rf_.checkForNewPacket()) {
    IthoCommand cmd = rf_.getLastCommand();
    String id_str = rf_.getLastIDstr();
    std::string id = std::string(id_str.c_str());
    
    int index = get_remote_index(id);
    
    if (index >= 0) {
      std::string room_name = remotes_[index].room_name;
      
      switch (cmd) {
        case IthoLow:
        case DucoLow:
          ESP_LOGD(TAG, "Received Low from %s", room_name.c_str());
          if (state_ != 1) {
            set_state(1, 0, room_name);
          } else {
            ESP_LOGD(TAG, "Already in Low state");
          }
          break;
        case IthoMedium:
        case DucoMedium:
          ESP_LOGD(TAG, "Received Medium from %s", room_name.c_str());
          if (state_ != 2) {
            set_state(2, 0, room_name);
          } else {
            ESP_LOGD(TAG, "Already in Medium state");
          }
          break;
        case IthoHigh:
        case DucoHigh:
          ESP_LOGD(TAG, "Received High from %s", room_name.c_str());
          if (state_ != 3) {
            set_state(3, 0, room_name);
          } else {
            ESP_LOGD(TAG, "Already in High state");
          }
          break;
        case IthoFull:
          ESP_LOGD(TAG, "Received Full from %s", room_name.c_str());
          if (state_ != 4) {
            set_state(4, 0, room_name);
          } else {
            ESP_LOGD(TAG, "Already in Full state");
          }
          break;
        case IthoTimer1:
          ESP_LOGD(TAG, "Received Timer1 from %s", room_name.c_str());
          if (state_ != 13) {
            set_state(13, TIME_10MIN, room_name);
          } else {
            ESP_LOGD(TAG, "Already in Timer1 state, resetting timer");
            set_state(13, TIME_10MIN, room_name);  // Reset timer even if already in timer mode
          }
          break;
        case IthoTimer2:
          ESP_LOGD(TAG, "Received Timer2 from %s", room_name.c_str());
          if (state_ != 23) {
            set_state(23, TIME_20MIN, room_name);
          } else {
            ESP_LOGD(TAG, "Already in Timer2 state, resetting timer");
            set_state(23, TIME_20MIN, room_name);  // Reset timer even if already in timer mode
          }
          break;
        case IthoTimer3:
          ESP_LOGD(TAG, "Received Timer3 from %s", room_name.c_str());
          if (state_ != 33) {
            set_state(33, TIME_30MIN, room_name);
          } else {
            ESP_LOGD(TAG, "Already in Timer3 state, resetting timer");
            set_state(33, TIME_30MIN, room_name);  // Reset timer even if already in timer mode
          }
          break;
        default:
          break;
      }
      
      if (fan_ != nullptr) {
        fan_->update_state();
      }
    } else {
      ESP_LOGV(TAG, "Ignored device-id: %s", id.c_str());
    }
  }
  
  interrupts();
}

// IthoFan Implementation

void IthoFan::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Itho Fan...");
  
  // Don't set initial state here, let on_boot handle it
  // This prevents conflicts with the on_boot action
}

void IthoFan::dump_config() {
  ESP_LOGCONFIG(TAG, "Itho Fan");
  LOG_FAN("", "Itho Fan", this);
}

fan::FanTraits IthoFan::get_traits() {
  auto traits = fan::FanTraits(false, true, false, 4);
  traits.set_supported_preset_modes({"Timer 10min", "Timer 20min", "Timer 30min"});
  return traits;
}

void IthoFan::control(const fan::FanCall &call) {
  if (hub_ == nullptr) {
    return;
  }
  
  if (call.get_state().has_value()) {
    this->state = *call.get_state();
    if (!this->state) {
      // Turn off = set to low
      hub_->send_command(1);
    }
  }
  
  if (call.get_speed().has_value()) {
    this->speed = *call.get_speed();
    hub_->send_command(this->speed);
  }
  
  if (call.get_preset_mode()) {
    const char* preset = call.get_preset_mode();
    if (strcmp(preset, "Timer 10min") == 0) {
      hub_->send_command(13);
    } else if (strcmp(preset, "Timer 20min") == 0) {
      hub_->send_command(23);
    } else if (strcmp(preset, "Timer 30min") == 0) {
      hub_->send_command(33);
    }
  }
  
  this->publish_state();
}

void IthoFan::update_state() {
  if (hub_ == nullptr) {
    return;
  }
  
  int state = hub_->get_state();
  
  // Update state directly without triggering control()
  // Map state to fan speed
  if (state >= 10) {
    // Timer mode - treat as high speed
    this->speed = 3;
    this->state = true;
    
    // Set preset based on timer state
    if (state == 13) {
      this->set_preset_mode_("Timer 10min");
    } else if (state == 23) {
      this->set_preset_mode_("Timer 20min");
    } else if (state == 33) {
      this->set_preset_mode_("Timer 30min");
    }
  } else {
    this->set_preset_mode_("");
    this->speed = state;
    this->state = state > 0;
  }
  
  // Publish the updated state to Home Assistant
  this->publish_state();
}

}  // namespace itho_fan
}  // namespace esphome
