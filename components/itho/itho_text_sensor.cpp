// Location of this file:
//  ./components/itho/itho_text_sensor.cpp
#include "itho_text_sensor.h"
#include "itho.h"

namespace esphome {
namespace itho {

// Used for referencing outside FanRecv Class
text_sensor::TextSensor *InstanceRefFanspeed;
bool InitRunned = false;

// helper-function 
String TextSensorfromState(int currentState)
{
    switch (currentState)
    {
      case 1:  return "Low";
      case 2:  return "Medium";
      case 3:  return "High";
      case 4:  return "Full";
      case 13:
      case 23:
      case 33: return "High(T)";
      default: return "Unknown";
    }
}


// Update timer every 1 second
FanRecv::FanRecv()
  : TextSensor(), PollingComponent()
  , fanspeed(new text_sensor::TextSensor())
  , fantimer(new text_sensor::TextSensor())
  , Lastid(new text_sensor::TextSensor())
{
  PollingComponent::set_update_interval(1000);
}


void FanRecv::setup()
{
  // Make textsensor outside class available, so it can be used in Interrupt Service Routine
  InstanceRefFanspeed = this->fanspeed;
  rf.init();

  // Following electronics wiring schema, change PIN if you wire differently
  pinMode(D1, INPUT);
  attachInterrupt(D1, ITHOinterrupt, FALLING);
  rf.initReceive();
  InitRunned = true;
}


void FanRecv::loop()
{
  // When Signal (from ISR) packet received, process packet
  if (ITHOhasPacket)
  {
    ITHOhasPacket = false;
    ITHOcheck();
  }
}


void FanRecv::update()
{
  if (State >= 10)
  {
    Timer--;
  }

  if ((State >= 10) && (Timer <= 0))
  {
    State = 1;
    Timer = 0;
    // his ensure that timer-value of 0 is published when elapsed
    fantimer->publish_state(String(Timer).c_str());
  }

  // Publish new data when vars are changed or timer is running
  if ((OldState != State) || (Timer > 0) || InitRunned)
  {
    fanspeed->publish_state(TextSensorfromState(State).c_str());
    fantimer->publish_state(String(Timer).c_str());
    Lastid->publish_state(LastID.c_str());
    OldState = State;
    InitRunned = false;
  }

  // ESP_LOGD("itho", "Update: State=%d, Timer=%d, LastID=%s", State, Timer, LastID.c_str());
}


void FanRecv::set_fanspeed(text_sensor::TextSensor *sensor) 
{ 
  this->fanspeed = sensor;
}


void FanRecv::set_fantimer(text_sensor::TextSensor *sensor) 
{ 
  this->fantimer = sensor;
}


void FanRecv::set_lastid(text_sensor::TextSensor *sensor) 
{ 
  this->Lastid = sensor;
}

}  // namespace itho
}  // namespace esphome
