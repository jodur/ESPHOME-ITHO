#include "esphome.h"
#include "IthoCC1101.h"


//List of States:

// 1 - Itho ventilation unit to lowest speed
// 2 - Itho ventilation unit to medium speed
// 3 - Itho ventilation unit to high speed
// 4 - Itho ventilation unit to full speed
// 13 -Itho to high speed with hardware timer (10 min)
// 23 -Itho to high speed with hardware timer (20 min)
// 33 -Itho to high speed with hardware timer (30 min)

typedef struct { String Id; String Roomname; } IdDict;

// Global struct to store Names, should be changed in boot call,to set user specific
IdDict Idlist[] = { {"ID1", "Controller Room1"},
					{"ID2",	"Controller Room2"},
					{"ID3",	"Controller Room3"}
				};
IthoCC1101 rf;
void ITHOinterrupt() ICACHE_RAM_ATTR;
void ITHOcheck();

// extra for interrupt handling
bool ITHOhasPacket = false;

// init states
int State=1; // after startup it is assumed that the fan is running low
int OldState=1;
int Timer=0;

// init ID's
String LastID;
String OldLastID;
String Mydeviceid = "ESPHOME"; // should be changed in boot call,to set user specific

long LastPublish=0; 
bool InitRunned = false;

// Timer values for hardware timer in Fan
#define Time1      10*60
#define Time2      20*60
#define Time3      30*60


TextSensor *InsReffanspeed; // Used for referencing outside FanRecv Class

String TextSensorfromState(int currentState)
{
	switch (currentState)
	{
	case 1: 
		return "Low";
		break;
	case 2:
		return "Medium";
		break;
	case 3: 
		return "High";
		break;
	case 13: case 23: case 33:
		return "High(T)";
		break;
	case 4: 
		return "Full";
		break;
	default:
	    return "Unknow";
		break;
	}
}


class FanRecv : public PollingComponent {
  public:

    // Publish 3 sensors
    // The state of the fan, Timer value and Last controller that issued the current state
    TextSensor *fanspeed = new TextSensor();
    // Timer left (though this is indicative) when pressing the timer button once, twice or three times
    TextSensor *fantimer = new TextSensor();
	// Last id that has issued the current state
	TextSensor *Lastid = new TextSensor();

    // Update timer 1 second
    FanRecv() : PollingComponent(1000) { }

    void setup()
	{
	  InsReffanspeed = this->fanspeed; // Make textsensor outside class available, so it can be used in Interrupt Service Routine
	  rf.init();
      // Followin wiring schema, change PIN if you wire differently
      pinMode(D1, INPUT);
      attachInterrupt(D1, ITHOinterrupt, FALLING);
      rf.initReceive();
	  InitRunned = true;
    }
    void loop() override
	{
		if (ITHOhasPacket)  // When Signal (from ISR) packet received, process packet 
		{
			ITHOhasPacket=  false;
			ITHOcheck();
		}
	}
    void update() override
	{
		if (State >= 10)
		{
			Timer--;
		}

		if ((State >= 10) && (Timer <= 0))
		{
			State = 1;
			Timer = 0;
			fantimer->publish_state(String(Timer).c_str()); // this ensures that times value 0 is published when elapsed
		}
		//Publish new data when vars are changed or timer is running
		if ((OldState != State) || (Timer > 0)|| InitRunned)
		{
			fanspeed->publish_state(TextSensorfromState(State).c_str());
			fantimer->publish_state(String(Timer).c_str());
			Lastid->publish_state(LastID.c_str());
			OldState = State;
			InitRunned = false;
		}
    }
};

// Figure out how to do multiple switches instead of duplicating them
// we need
// send: low, medium, high, full
//       timer 1 (10 minutes), 2 (20), 3 (30)
// To optimize testing, reset published state immediately so you can retrigger (i.e. momentarily button press)


class FanSendLow : public Component, public Switch {
public:

	void write_state(bool state) override {
		if (state) {
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
};

class FanSendMedium : public Component, public Switch {
  public:

    void write_state(bool state) override {
      if ( state ) {
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
};

class FanSendHigh : public Component, public Switch {
public:

	void write_state(bool state) override {
		if (state) {
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
};

class FanSendFull : public Component, public Switch {
public:

	void write_state(bool state) override {
		if (state) {
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
};

class FanSendIthoTimer1 : public Component, public Switch {
  public:

    void write_state(bool state) override {
      if ( state ) {
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
};

class FanSendIthoTimer2 : public Component, public Switch {
  public:

    void write_state(bool state) override {
      if ( state ) {
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
};

class FanSendIthoTimer3 : public Component, public Switch {
  public:

    void write_state(bool state) override {
      if ( state ) {
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
};

class FanSendIthoJoin : public Component, public Switch {
  public:

    void write_state(bool state) override {
      if ( state ) {
		noInterrupts();  
        rf.sendCommand(IthoJoin);
		interrupts();
		rf.initReceive();
        State = 1111;
        Timer = 0;
        publish_state(!state);
      }
    }
};


void ITHOinterrupt()
{
	// Signal ITHO received  something
	ITHOhasPacket = true;
}


int RFRemoteIndex(String rfremoteid)
{
	if (rfremoteid == Idlist[0].Id) return 0;
	else if (rfremoteid == Idlist[1].Id) return 1;
	else if (rfremoteid == Idlist[2].Id) return 2;
	else return -1;
}


void ITHOcheck() {
  noInterrupts(); // disable new interrupts
  if (rf.checkForNewPacket()) {
    IthoCommand cmd = rf.getLastCommand();
	String Id = rf.getLastIDstr();
	int index = RFRemoteIndex(Id);
	if ( index>=0) { // Only accept commands that are in the list
		switch (cmd) {
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
		}
	}
	else ESP_LOGV("","Ignored device-id: %s", Id.c_str());
  }
  interrupts(); //enable interrupts
}
