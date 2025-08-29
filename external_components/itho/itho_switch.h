// Location of this file:
//  ./components/itho/itho_switch.h
#pragma once
#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"

// We send commands to itho by deriving from 'switch'.
// Each command has a separate switch (a separate class).

namespace esphome {
namespace itho {

class FanSendLow : public switch_::Switch, public Component {
 public:
  void setup() override {}
  void write_state(bool state) override;
};


class FanSendMedium : public switch_::Switch, public Component {
 public:
 void setup() override {}
 void write_state(bool state) override;
};


class FanSendHigh : public switch_::Switch, public Component {
  public:
  void setup() override {}
  void write_state(bool state) override;
 };
 

class FanSendFull : public switch_::Switch, public Component {
  public:
  void setup() override {}
  void write_state(bool state) override;
 };
 

 class FanSendIthoTimer1 : public switch_::Switch, public Component {
 public:
 void setup() override {}
 void write_state(bool state) override;
};


class FanSendIthoTimer2 : public switch_::Switch, public Component {
 public:
 void setup() override {}
 void write_state(bool state) override;
};


class FanSendIthoTimer3 : public switch_::Switch, public Component {
 public:
 void setup() override {}
 void write_state(bool state) override;
};


class FanSendIthoJoin : public switch_::Switch, public Component{
 public:
 void setup() override {}
 void write_state(bool state) override;
};

}  // namespace itho
}  // namespace esphome
