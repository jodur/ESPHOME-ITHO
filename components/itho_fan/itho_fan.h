#pragma once

#include "esphome/core/component.h"
#include "esphome/components/fan/fan.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/hal.h"
#include "esphome/core/gpio.h"
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
  void set_device_name(const std::string &name) { device_name_ = name; }
  void add_remote_id(const std::string &id, const std::string &room_name);
  void set_interrupt_pin(InternalGPIOPin *pin) { interrupt_pin_ = pin; }
  
  void send_command(uint8_t command);
  void send_join();
  void send_leave();
  
  int get_state() const { return state_; }
  int get_timer() const { return timer_; }
  std::string get_last_id() const { return last_id_; }
  InternalGPIOPin *get_interrupt_pin() const { return interrupt_pin_; }
  
  void register_fan(class IthoFan *fan) { fan_ = fan; }
  void register_state_sensor(sensor::Sensor *sensor) { state_sensor_ = sensor; }
  void register_speed_sensor(text_sensor::TextSensor *sensor) { speed_sensor_ = sensor; }
  void register_timer_sensor(text_sensor::TextSensor *sensor) { timer_sensor_ = sensor; }
  void register_controller_sensor(text_sensor::TextSensor *sensor) { controller_sensor_ = sensor; }
  void publish_sensors();
  void on_homeassistant_connected();
  
  // Public member accessed by interrupt handler
  bool has_packet_{false};

 protected:
  friend void IRAM_ATTR itho_interrupt_handler();
  
  void process_packet();
  int get_remote_index(const std::string &id);
  void set_state(int state, int timer, const std::string &id);
  
  IthoCC1101 rf_;
  std::vector<RemoteInfo> remotes_;
  std::string device_id_{"ESPHOME"};
  std::string device_name_{"ESPHome"};
  InternalGPIOPin *interrupt_pin_{nullptr};
  
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
