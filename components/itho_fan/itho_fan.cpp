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

// Number of transmissions and delay between them
static const uint8_t COMMAND_REPEAT = 3;
static const uint8_t COMMAND_DELAY_MS = 40;
static const uint8_t JOIN_REPEAT = 10;
static const uint8_t JOIN_DELAY_MS = 40;
static const uint8_t LEAVE_REPEAT = 30;  // leave must be transmitted for ~1 second
static const uint8_t LEAVE_DELAY_MS = 4;
static const size_t MAX_TX_QUEUE = 8;

// IthoFanHub Implementation

void IthoFanHub::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Itho Fan Hub...");
  if (radio_ != nullptr) {
    radio_->register_listener(this);
  }
  init_complete_ = true;
  last_id_ = "System";
}

void IthoFanHub::loop() {
  process_tx_queue();
  
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
  ESP_LOGCONFIG(TAG, "  Device ID: %u,%u,%u", device_id_[0], device_id_[1], device_id_[2]);
  ESP_LOGCONFIG(TAG, "  Device Name: %s", device_name_.c_str());
  ESP_LOGCONFIG(TAG, "  Registered Remotes: %d", remotes_.size());
  for (const auto &remote : remotes_) {
    ESP_LOGCONFIG(TAG, "    - ID: %s, Room: %s", remote.id.c_str(), remote.room_name.c_str());
  }
}

void IthoFanHub::set_device_id(uint8_t id1, uint8_t id2, uint8_t id3) {
  device_id_[0] = id1;
  device_id_[1] = id2;
  device_id_[2] = id3;
}

void IthoFanHub::add_remote_id(const std::string &id, const std::string &room_name) {
  remotes_.push_back({id, room_name});
}

void IthoFanHub::transmit(IthoCommand command, uint8_t repeat, uint8_t delay_ms) {
  if (radio_ == nullptr) {
    ESP_LOGE(TAG, "No cc1101 radio configured");
    return;
  }
  if (tx_queue_.size() >= MAX_TX_QUEUE) {
    ESP_LOGW(TAG, "Transmit queue full, command dropped");
    return;
  }
  packet_counter_++;
  tx_queue_.push_back({itho_build_packet(command, device_id_, packet_counter_), repeat, delay_ms});
  process_tx_queue();
}

void IthoFanHub::process_tx_queue() {
  if (tx_queue_.empty() || (int32_t) (millis() - next_tx_) < 0) {
    return;
  }
  Transmission &tx = tx_queue_.front();
  if (radio_->transmit_packet(tx.packet) != cc1101::CC1101Error::NONE) {
    ESP_LOGW(TAG, "Transmission failed");
  }
  next_tx_ = millis() + tx.delay_ms;
  if (--tx.remaining == 0) {
    tx_queue_.pop_front();
  }
}

void IthoFanHub::send_command(uint8_t command) {
  switch (command) {
    case 1:
      transmit(ITHO_LOW, COMMAND_REPEAT, COMMAND_DELAY_MS);
      set_state(1, 0, device_name_);
      break;
    case 2:
      transmit(ITHO_MEDIUM, COMMAND_REPEAT, COMMAND_DELAY_MS);
      set_state(2, 0, device_name_);
      break;
    case 3:
      transmit(ITHO_HIGH, COMMAND_REPEAT, COMMAND_DELAY_MS);
      set_state(3, 0, device_name_);
      break;
    case 4:
      transmit(ITHO_FULL, COMMAND_REPEAT, COMMAND_DELAY_MS);
      set_state(4, 0, device_name_);
      break;
    case 13:
      transmit(ITHO_TIMER1, COMMAND_REPEAT, COMMAND_DELAY_MS);
      set_state(13, TIME_10MIN, device_name_);
      break;
    case 23:
      transmit(ITHO_TIMER2, COMMAND_REPEAT, COMMAND_DELAY_MS);
      set_state(23, TIME_20MIN, device_name_);
      break;
    case 33:
      transmit(ITHO_TIMER3, COMMAND_REPEAT, COMMAND_DELAY_MS);
      set_state(33, TIME_30MIN, device_name_);
      break;
    default:
      ESP_LOGW(TAG, "Unknown command %u", command);
      return;
  }

  if (fan_ != nullptr) {
    fan_->update_state();
  }
}

void IthoFanHub::send_join() {
  transmit(ITHO_JOIN, JOIN_REPEAT, JOIN_DELAY_MS);
  ESP_LOGI(TAG, "Join command sent");
}

void IthoFanHub::send_leave() {
  transmit(ITHO_LEAVE, LEAVE_REPEAT, LEAVE_DELAY_MS);
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

void IthoFanHub::on_packet(const std::vector<uint8_t> &packet, float freq_offset, float rssi, uint8_t lqi) {
  IthoMessage msg;
  if (!itho_decode(packet, msg)) {
    return;  // not an Itho message
  }
  IthoCommand cmd = itho_parse_command(msg);

  // Remote ID in the same notation as before (ITHO-Lib getLastIDstr): hex without leading zeros
  char id[12];
  snprintf(id, sizeof(id), "%x,%x,%x", msg.device_id()[0], msg.device_id()[1], msg.device_id()[2]);
  ESP_LOGV(TAG, "Packet from %s, command %u, rssi %.1f dBm", id, cmd, rssi);

  int index = get_remote_index(id);
  if (index < 0) {
    ESP_LOGD(TAG, "Ignored device-id: %s", id);
    return;
  }
  const std::string &room_name = remotes_[index].room_name;

  int state;
  int timer = 0;
  const char *name;
  switch (cmd) {
    case ITHO_LOW:
      state = 1;
      name = "Low";
      break;
    case ITHO_MEDIUM:
      state = 2;
      name = "Medium";
      break;
    case ITHO_HIGH:
      state = 3;
      name = "High";
      break;
    case ITHO_FULL:
      state = 4;
      name = "Full";
      break;
    case ITHO_TIMER1:
      state = 13;
      timer = TIME_10MIN;
      name = "Timer1";
      break;
    case ITHO_TIMER2:
      state = 23;
      timer = TIME_20MIN;
      name = "Timer2";
      break;
    case ITHO_TIMER3:
      state = 33;
      timer = TIME_30MIN;
      name = "Timer3";
      break;
    default:
      return;
  }

  ESP_LOGD(TAG, "Received %s from %s", name, room_name.c_str());
  if (state_ == state && timer == 0) {
    ESP_LOGD(TAG, "Already in %s state", name);
    return;
  }
  // Timer commands always (re)start the timer
  set_state(state, timer, room_name);

  if (fan_ != nullptr) {
    fan_->update_state();
  }
}

// IthoFan Implementation

void IthoFan::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Itho Fan...");
  this->set_supported_preset_modes({"Timer 10min", "Timer 20min", "Timer 30min"});
  
  // Don't set initial state here, let on_boot handle it
  // This prevents conflicts with the on_boot action
}

void IthoFan::dump_config() {
  ESP_LOGCONFIG(TAG, "Itho Fan");
  LOG_FAN("", "Itho Fan", this);
}

fan::FanTraits IthoFan::get_traits() {
  return fan::FanTraits(false, true, false, 4);
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
