// Location of this file:
//  ./components/itho/itho_text_sensor.h
#pragma once
#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"

// We present status of the itho to the user deriving from 'text_sensor'.
// A single class is used for 3 text-values.

namespace esphome {
namespace itho {

// Used for referencing outside FanRecv Class
extern text_sensor::TextSensor *InstanceRefFanspeed;

class FanRecv : public text_sensor::TextSensor, public PollingComponent {
 public:
  FanRecv();
  void setup() override;
  void loop() override;
  void update();

  void set_fanspeed(text_sensor::TextSensor *sensor);
  void set_fantimer(text_sensor::TextSensor *sensor);
  void set_lastid(text_sensor::TextSensor *sensor);

 protected:
  // Publish 3 sensors
  text_sensor::TextSensor *fanspeed;  // The state of the fan
  text_sensor::TextSensor *fantimer;  // Timer left when pressing the timer button
  text_sensor::TextSensor *Lastid;    // Last id that has issued the current state
};

}  // namespace itho
}  // namespace esphome
