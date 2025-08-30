// Location of this file:
//  ./external_components/itho/itho.cpp
#include "itho.h"

// Global struct to store Names, should be changed in boot call, to set user specific
IdDict Idlist[] = { {"ID1", "Controller Room1"},
                    {"ID2",	"Controller Room2"},
                    {"ID3",	"Controller Room3"}
                  };
IthoCC1101 rf;

// extra for interrupt handling
bool ITHOhasPacket = false;

// init states
int State = 1; // after startup it is assumed that the fan is running low
int OldState = 1;
int Timer = 0;

// init ID's
String LastID;
String Mydeviceid = "ESPHOME"; // should be changed in boot call or yaml, to set user specific


void ITHOinterrupt()
{
	// Signal to itho_text_sensor that itho received something
	ITHOhasPacket = true;
}


int RFRemoteIndex(String rfremoteid)
{
  if (rfremoteid == Idlist[0].Id) return 0;
  else if (rfremoteid == Idlist[1].Id) return 1;
  else if (rfremoteid == Idlist[2].Id) return 2;
  else return -1;
}


void ITHOcheck()
{
  // temporarily disable new interrupts
  noInterrupts();

  if (rf.checkForNewPacket())
  {
    IthoCommand cmd = rf.getLastCommand();
    String Id = rf.getLastIDstr();
    int index = RFRemoteIndex(Id);
    if (index >= 0) 
    {
      // Only accept commands that are in the list
      switch (cmd)
      {
        case IthoUnknown:
          ESP_LOGV("custom", "Unknown command");
          break;

        case IthoLow:
        case DucoLow:
          ESP_LOGD("custom", "IthoLow");
          State = 1;
          Timer = 0;
          LastID = Idlist[index].Roomname;
          break;

        case IthoMedium:
        case DucoMedium:
          ESP_LOGD("custom", "Medium");
          State = 2;
          Timer = 0;
          LastID = Idlist[index].Roomname;
          break;

        case IthoHigh:
        case DucoHigh:
          ESP_LOGD("custom", "High");
          State = 3;
          Timer = 0;
          LastID = Idlist[index].Roomname;
          break;

        case IthoFull:
          ESP_LOGD("custom", "Full");
          State = 4;
          Timer = 0;
          LastID = Idlist[index].Roomname;
          break;

        case IthoTimer1:
          ESP_LOGD("custom", "Timer1");
          State = 13;
          Timer = Time1;
          LastID = Idlist[index].Roomname;
          break;

        case IthoTimer2:
          ESP_LOGD("custom", "Timer2");
          State = 23;
          Timer = Time2;
          LastID = Idlist[index].Roomname;
          break;

        case IthoTimer3:
          ESP_LOGD("custom", "Timer3");
          State = 33;
          Timer = Time3;
          LastID = Idlist[index].Roomname;
          break;

        case IthoJoin:
          break;

        case IthoLeave:
          break;

        default:
          break;
      }
    }
    else
      ESP_LOGV("","Ignored device-id: %s", Id.c_str());
  }

  // enable interrupts again
  interrupts();
}
