#pragma once

#include "esphome/core/component.h"
#include "esphome/components/fan/fan.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/cc1101/cc1101.h"
#include "itho_protocol.h"
#include <deque>
#include <vector>
#include <string>

namespace esphome {
namespace itho_fan {

struct RemoteInfo {
  std::string id;
  std::string room_name;
};

class IthoFanHub : public Component, public cc1101::CC1101Listener {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  
  void set_device_id(uint8_t id1, uint8_t id2, uint8_t id3);
  void set_device_name(const std::string &name) { device_name_ = name; }
  void add_remote_id(const std::string &id, const std::string &room_name);
  void set_radio(cc1101::CC1101Component *radio) { radio_ = radio; }

  // Called by the cc1101 component for every received packet
  void on_packet(const std::vector<uint8_t> &packet, float freq_offset, float rssi, uint8_t lqi) override;
  
  void send_command(uint8_t command);
  void send_join();
  void send_leave();
  
  int get_state() const { return state_; }
  int get_timer() const { return timer_; }
  std::string get_last_id() const { return last_id_; }
  
  void register_fan(class IthoFan *fan) { fan_ = fan; }
  void register_state_sensor(sensor::Sensor *sensor) { state_sensor_ = sensor; }
  void register_speed_sensor(text_sensor::TextSensor *sensor) { speed_sensor_ = sensor; }
  void register_timer_sensor(text_sensor::TextSensor *sensor) { timer_sensor_ = sensor; }
  void register_controller_sensor(text_sensor::TextSensor *sensor) { controller_sensor_ = sensor; }
  void publish_sensors();
  void on_homeassistant_connected();

 protected:
  struct Transmission {
    std::vector<uint8_t> packet;
    uint8_t remaining;
    uint8_t delay_ms;
  };

  void transmit(IthoCommand command, uint8_t repeat, uint8_t delay_ms);
  void process_tx_queue();
  int get_remote_index(const std::string &id);
  void set_state(int state, int timer, const std::string &id);

  cc1101::CC1101Component *radio_{nullptr};
  std::deque<Transmission> tx_queue_;
  uint32_t next_tx_{0};
  uint8_t packet_counter_{0};

  std::vector<RemoteInfo> remotes_;
  uint8_t device_id_[3]{10, 87, 81};
  std::string device_name_{"ESPHome"};
  
  int state_{1};
  int old_state_{1};
  int timer_{0};
  std::string last_id_{"System"};
  
  bool init_complete_{false};
  
  class IthoFan *fan_{nullptr};
  sensor::Sensor *state_sensor_{nullptr};
  text_sensor::TextSensor *speed_sensor_{nullptr};
  text_sensor::TextSensor *timer_sensor_{nullptr};
  text_sensor::TextSensor *controller_sensor_{nullptr};
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
