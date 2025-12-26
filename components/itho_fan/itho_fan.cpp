#include "itho_fan.h"
#include "esphome/core/log.h"

namespace esphome {
namespace itho_fan {

static const char *const TAG = "itho_fan";

// Timer constants
static const int TIME_10MIN = 10 * 60;
static const int TIME_20MIN = 20 * 60;
static const int TIME_30MIN = 30 * 60;

// Itho message command bytes (from ITHO-Lib)
static const uint8_t ITHO_MSG_HIGH[] = {34, 241, 3, 0, 4, 4};
static const uint8_t ITHO_MSG_FULL[] = {34, 241, 3, 0, 4, 4};
static const uint8_t ITHO_MSG_MEDIUM[] = {34, 241, 3, 0, 3, 4};
static const uint8_t ITHO_MSG_LOW[] = {34, 241, 3, 0, 2, 4};
static const uint8_t ITHO_MSG_TIMER1[] = {34, 243, 3, 0, 0, 10};
static const uint8_t ITHO_MSG_TIMER2[] = {34, 243, 3, 0, 0, 20};
static const uint8_t ITHO_MSG_TIMER3[] = {34, 243, 3, 0, 0, 30};
static const uint8_t ITHO_MSG_JOIN[] = {31, 201, 12, 0, 34, 241};
static const uint8_t ITHO_MSG_LEAVE[] = {31, 201, 6, 0, 31, 201};

// IthoFanHub Implementation

void IthoFanHub::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Itho Fan Hub...");
  
  if (cc1101_ == nullptr) {
    ESP_LOGE(TAG, "CC1101 component not set!");
    this->mark_failed();
    return;
  }
  
  // Subscribe to CC1101 packet reception
  cc1101_->get_packet_trigger()->add_callback(
      [this](std::vector<uint8_t> packet, float rssi, uint8_t lqi) {
        this->handle_packet(packet, rssi, lqi);
      });
  
  ESP_LOGCONFIG(TAG, "Itho Fan Hub setup complete");
}

void IthoFanHub::loop() {
  // Handle timer countdown
  static uint32_t last_update = 0;
  uint32_t now = millis();
  
  if (now - last_update >= 1000) {
    last_update = now;
    
    if (state_ >= 10 && timer_ > 0) {
      timer_--;
      
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
  ESP_LOGCONFIG(TAG, "  Device ID: %d,%d,%d", device_id_[0], device_id_[1], device_id_[2]);
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

void IthoFanHub::send_command(IthoCommand command) {
  if (cc1101_ == nullptr) {
    ESP_LOGW(TAG, "CC1101 not available, cannot send command");
    return;
  }
  
  std::vector<uint8_t> packet = encode_packet(command);
  
  // Update local state before sending
  switch (command) {
    case IthoCommand::LOW:
      set_state(1, 0, "System");
      break;
    case IthoCommand::MEDIUM:
      set_state(2, 0, "System");
      break;
    case IthoCommand::HIGH:
      set_state(3, 0, "System");
      break;
    case IthoCommand::FULL:
      set_state(4, 0, "System");
      break;
    case IthoCommand::TIMER1:
      set_state(13, TIME_10MIN, "System");
      break;
    case IthoCommand::TIMER2:
      set_state(23, TIME_20MIN, "System");
      break;
    case IthoCommand::TIMER3:
      set_state(33, TIME_30MIN, "System");
      break;
    default:
      break;
  }
  
  // Send packet via CC1101
  auto result = cc1101_->transmit_packet(packet);
  if (result != cc1101::CC1101Error::NONE) {
    ESP_LOGW(TAG, "Failed to transmit packet");
  }
  
  if (fan_ != nullptr) {
    fan_->update_state();
  }
}

void IthoFanHub::set_state(int state, int timer, const std::string &id) {
  state_ = state;
  timer_ = timer;
  last_id_ = id;
}

int IthoFanHub::get_remote_index(const std::string &id) {
  for (size_t i = 0; i < remotes_.size(); i++) {
    if (remotes_[i].id == id) {
      return i;
    }
  }
  return -1;
}

void IthoFanHub::handle_packet(std::vector<uint8_t> packet, float rssi, uint8_t lqi) {
  std::string device_id;
  IthoCommand cmd = decode_packet(packet, device_id);
  
  if (cmd == IthoCommand::UNKNOWN) {
    ESP_LOGV(TAG, "Unknown or invalid packet received");
    return;
  }
  
  int index = get_remote_index(device_id);
  
  if (index >= 0) {
    std::string room_name = remotes_[index].room_name;
    
    ESP_LOGD(TAG, "Received command from %s (RSSI: %.1f dBm)", room_name.c_str(), rssi);
    
    switch (cmd) {
      case IthoCommand::LOW:
        set_state(1, 0, room_name);
        break;
      case IthoCommand::MEDIUM:
        set_state(2, 0, room_name);
        break;
      case IthoCommand::HIGH:
        set_state(3, 0, room_name);
        break;
      case IthoCommand::FULL:
        set_state(4, 0, room_name);
        break;
      case IthoCommand::TIMER1:
        set_state(13, TIME_10MIN, room_name);
        break;
      case IthoCommand::TIMER2:
        set_state(23, TIME_20MIN, room_name);
        break;
      case IthoCommand::TIMER3:
        set_state(33, TIME_30MIN, room_name);
        break;
      default:
        break;
    }
    
    if (fan_ != nullptr) {
      fan_->update_state();
    }
  } else {
    ESP_LOGV(TAG, "Ignored device-id: %s", device_id.c_str());
  }
}

std::vector<uint8_t> IthoFanHub::encode_packet(IthoCommand command) {
  std::vector<uint8_t> packet;
  
  // Simplified Itho packet encoding
  // Full implementation would need proper Manchester encoding and checksum
  // This is a basic structure based on ITHO-Lib
  
  // Device type and ID
  packet.push_back(148);  // Start byte
  packet.push_back(device_id_[0]);
  packet.push_back(device_id_[1]);
  packet.push_back(device_id_[2]);
  
  // Counter
  counter_++;
  packet.push_back(counter_);
  
  // Command bytes
  const uint8_t *cmd_bytes = nullptr;
  size_t cmd_len = 6;
  
  switch (command) {
    case IthoCommand::LOW:
      cmd_bytes = ITHO_MSG_LOW;
      break;
    case IthoCommand::MEDIUM:
      cmd_bytes = ITHO_MSG_MEDIUM;
      break;
    case IthoCommand::HIGH:
      cmd_bytes = ITHO_MSG_HIGH;
      break;
    case IthoCommand::FULL:
      cmd_bytes = ITHO_MSG_FULL;
      break;
    case IthoCommand::TIMER1:
      cmd_bytes = ITHO_MSG_TIMER1;
      break;
    case IthoCommand::TIMER2:
      cmd_bytes = ITHO_MSG_TIMER2;
      break;
    case IthoCommand::TIMER3:
      cmd_bytes = ITHO_MSG_TIMER3;
      break;
    case IthoCommand::JOIN:
      cmd_bytes = ITHO_MSG_JOIN;
      break;
    case IthoCommand::LEAVE:
      cmd_bytes = ITHO_MSG_LEAVE;
      break;
    default:
      cmd_bytes = ITHO_MSG_LOW;
      break;
  }
  
  if (cmd_bytes != nullptr) {
    for (size_t i = 0; i < cmd_len; i++) {
      packet.push_back(cmd_bytes[i]);
    }
  }
  
  // TODO: Add proper Manchester encoding and checksum calculation
  // For now, this is a simplified version
  
  return packet;
}

IthoCommand IthoFanHub::decode_packet(const std::vector<uint8_t> &packet, std::string &device_id) {
  // Simplified decoding - full implementation needs Manchester decoding
  // This is a placeholder that extracts basic information
  
  if (packet.size() < 10) {
    return IthoCommand::UNKNOWN;
  }
  
  // Extract device ID (simplified)
  char id_buf[32];
  snprintf(id_buf, sizeof(id_buf), "%d,%d,%d", packet[1], packet[2], packet[3]);
  device_id = id_buf;
  
  // Try to match command bytes
  // This is very simplified - real implementation needs proper decoding
  for (size_t i = 0; i < packet.size() - 6; i++) {
    if (memcmp(&packet[i], ITHO_MSG_LOW, 6) == 0) return IthoCommand::LOW;
    if (memcmp(&packet[i], ITHO_MSG_MEDIUM, 6) == 0) return IthoCommand::MEDIUM;
    if (memcmp(&packet[i], ITHO_MSG_HIGH, 6) == 0) return IthoCommand::HIGH;
    if (memcmp(&packet[i], ITHO_MSG_FULL, 6) == 0) return IthoCommand::FULL;
    if (memcmp(&packet[i], ITHO_MSG_TIMER1, 6) == 0) return IthoCommand::TIMER1;
    if (memcmp(&packet[i], ITHO_MSG_TIMER2, 6) == 0) return IthoCommand::TIMER2;
    if (memcmp(&packet[i], ITHO_MSG_TIMER3, 6) == 0) return IthoCommand::TIMER3;
  }
  
  return IthoCommand::UNKNOWN;
}

// IthoFan Implementation

void IthoFan::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Itho Fan...");
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
      hub_->send_command(IthoCommand::LOW);
    }
  }
  
  if (call.get_speed().has_value()) {
    int speed = *call.get_speed();
    switch (speed) {
      case 1:
        hub_->send_command(IthoCommand::LOW);
        break;
      case 2:
        hub_->send_command(IthoCommand::MEDIUM);
        break;
      case 3:
        hub_->send_command(IthoCommand::HIGH);
        break;
      case 4:
        hub_->send_command(IthoCommand::FULL);
        break;
    }
  }
  
  if (call.get_preset_mode()) {
    const char* preset = call.get_preset_mode();
    if (strcmp(preset, "Timer 10min") == 0) {
      hub_->send_command(IthoCommand::TIMER1);
    } else if (strcmp(preset, "Timer 20min") == 0) {
      hub_->send_command(IthoCommand::TIMER2);
    } else if (strcmp(preset, "Timer 30min") == 0) {
      hub_->send_command(IthoCommand::TIMER3);
    }
  }
  
  this->publish_state();
}

void IthoFan::update_state() {
  if (hub_ == nullptr) {
    return;
  }
  
  int state = hub_->get_state();
  
  // Create a fan call to update state
  auto call = this->make_call();
  
  // Map state to fan speed
  if (state >= 10) {
    // Timer mode - treat as high speed
    call.set_speed(3);
    
    // Set preset based on timer state
    if (state == 13) {
      call.set_preset_mode("Timer 10min");
    } else if (state == 23) {
      call.set_preset_mode("Timer 20min");
    } else if (state == 33) {
      call.set_preset_mode("Timer 30min");
    }
  } else {
    call.set_preset_mode("");
    call.set_speed(state);
  }
  
  call.set_state(state > 0);
  call.perform();
}

}  // namespace itho_fan
}  // namespace esphome
