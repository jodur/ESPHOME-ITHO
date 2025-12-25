#pragma once

#include "esphome/core/component.h"
#include "esphome/components/fan/fan.h"
#include "esphome/core/hal.h"
#include "IthoCC1101.h"
#include <vector>
#include <string>

namespace esphome {
namespace itho_fan {

struct RemoteInfo {
  std::string id;
  std::string room_name;
};

class IthoFanHub : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  
  void set_device_id(uint8_t id1, uint8_t id2, uint8_t id3);
  void add_remote_id(const std::string &id, const std::string &room_name);
  void set_interrupt_pin(uint8_t pin);
  
  void send_command(uint8_t command);
  
  int get_state() const { return state_; }
  int get_timer() const { return timer_; }
  std::string get_last_id() const { return last_id_; }
  
  void register_fan(class IthoFan *fan) { fan_ = fan; }

 protected:
  friend void IRAM_ATTR itho_interrupt_handler();
  
  void process_packet();
  int get_remote_index(const std::string &id);
  void set_state(int state, int timer, const std::string &id);
  
  IthoCC1101 rf_;
  std::vector<RemoteInfo> remotes_;
  std::string device_id_{"ESPHOME"};
  uint8_t interrupt_pin_{0};
  
  int state_{1};
  int old_state_{1};
  int timer_{0};
  std::string last_id_;
  
  bool has_packet_{false};
  bool init_complete_{false};
  
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
