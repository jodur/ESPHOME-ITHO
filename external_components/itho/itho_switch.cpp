// Location of this file:
//  ./external_components/itho/itho_switch.cpp
#include "itho_switch.h"
#include "itho.h"

namespace esphome {
namespace itho {

// TODO: Figure out how to do multiple switches instead of duplicating them.
// We need to send:
//   low, medium, high, full
//   timer 1 (10 minutes), 2 (20), 3 (30)
// To optimize testing, reset published state immediately so you can retrigger (i.e. momentarily button press)

void FanSendLow::write_state(bool state)
{
  if (state) 
  {
    noInterrupts();
    rf.sendCommand(IthoLow);
    interrupts();
    rf.initReceive();
    State = 1;
    Timer = 0;
    LastID = Mydeviceid;
    publish_state(!state);
  }
}


void FanSendMedium::write_state(bool state)
{
  if (state)
  {
    noInterrupts();  
    rf.sendCommand(IthoMedium);
    interrupts();
    rf.initReceive();
    State = 2;
    Timer = 0;
    LastID = Mydeviceid;
    publish_state(!state);
  }
}


void FanSendHigh::write_state(bool state)
{
  if (state)
  {
    noInterrupts();
    rf.sendCommand(IthoHigh);
    interrupts();
    rf.initReceive();
    State = 3;
    Timer = 0;
    LastID = Mydeviceid;
    publish_state(!state);
  }
}


void FanSendFull::write_state(bool state)
{
  if (state)
  {
    noInterrupts();
    rf.sendCommand(IthoFull);
    interrupts();
    rf.initReceive();
    State = 4;
    Timer = 0;
    LastID = Mydeviceid;
    publish_state(!state);
  }
}


void FanSendIthoTimer1::write_state(bool state)
{
  if (state)
  {
    noInterrupts();  
    rf.sendCommand(IthoTimer1);
    interrupts();
    rf.initReceive();
    State = 13;
    Timer = Time1;
    LastID = Mydeviceid;
    publish_state(!state);
  }
}


void FanSendIthoTimer2::write_state(bool state)
{
  if (state)
  {
    noInterrupts();  
    rf.sendCommand(IthoTimer2);
    interrupts();
    rf.initReceive();
    State = 23;
    Timer = Time2;
    LastID = Mydeviceid;
    publish_state(!state);
  }
}


void FanSendIthoTimer3::write_state(bool state)
{
  if (state)
  {
    noInterrupts();  
    rf.sendCommand(IthoTimer3);
    interrupts();
    rf.initReceive();
    State = 33;
    Timer = Time3;
    LastID = Mydeviceid;
    publish_state(!state);
  }
}


void FanSendIthoJoin::write_state(bool state)
{
  if (state)
  {
    noInterrupts();  
    rf.sendCommand(IthoJoin);
    interrupts();
    rf.initReceive();
    State = 1111;
    Timer = 0;
    publish_state(!state);
  }
}

}  // namespace itho
}  // namespace esphome
