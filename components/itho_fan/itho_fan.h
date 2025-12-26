#pragma once

#include "esphome/core/component.h"
#include "esphome/components/fan/fan.h"
#include "esphome/components/cc1101/cc1101.h"
#include <vector>
#include <string>

namespace esphome {
namespace itho_fan {

// Itho Command Definitions
enum class IthoCommand : uint8_t {
  UNKNOWN = 0,
  JOIN = 1,
  LEAVE = 2,
  STANDBY = 3,
  LOW = 4,
  MEDIUM = 5,
  HIGH = 6,
  FULL = 7,
  TIMER1 = 8,
  TIMER2 = 9,
  TIMER3 = 10,
};

struct RemoteInfo {
  std::string id;
  std::string room_name;
};

class IthoFanHub : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  
  void set_cc1101(cc1101::CC1101Component *cc1101) { cc1101_ = cc1101; }
  void set_device_id(uint8_t id1, uint8_t id2, uint8_t id3);
  void add_remote_id(const std::string &id, const std::string &room_name);
  
  void send_command(IthoCommand command);
  
  int get_state() const { return state_; }
  int get_timer() const { return timer_; }
  std::string get_last_id() const { return last_id_; }
  
  void register_fan(class IthoFan *fan) { fan_ = fan; }

 protected:
  void handle_packet(std::vector<uint8_t> packet, float rssi, uint8_t lqi);
  int get_remote_index(const std::string &id);
  void set_state(int state, int timer, const std::string &id);
  
  // Packet encoding/decoding
  std::vector<uint8_t> encode_packet(IthoCommand command);
  IthoCommand decode_packet(const std::vector<uint8_t> &packet, std::string &device_id);
  
  cc1101::CC1101Component *cc1101_{nullptr};
  std::vector<RemoteInfo> remotes_;
  uint8_t device_id_[3]{0, 0, 0};
  uint8_t counter_{0};
  
  int state_{1};
  int timer_{0};
  std::string last_id_{"System"};
  
  class IthoFan *fan_{nullptr};
};

class IthoFan : public Component, public fan::Fan {
 public:
  void setup() override;
  void dump_config() override;
  
  void set_hub(IthoFanHub *hub) { hub_ = hub; }
  
  fan::FanTraits get_traits() override;
  
  void update_state();

 protected:
  void control(const fan::FanCall &call) override;
  
  IthoFanHub *hub_{nullptr};
};

}  // namespace itho_fan
}  // namespace esphome
