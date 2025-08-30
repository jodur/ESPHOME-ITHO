// Location of this file:
//  ./external_components/itho/itho.h
#pragma once
#include "esphome.h"
#include "IthoCC1101.h"

// ----- List of States: -----
// 1 - Itho ventilation unit to lowest speed
// 2 - Itho ventilation unit to medium speed
// 3 - Itho ventilation unit to high speed
// 4 - Itho ventilation unit to full speed
// 13 -Itho to high speed with hardware timer (10 min)
// 23 -Itho to high speed with hardware timer (20 min)
// 33 -Itho to high speed with hardware timer (30 min)

// Timer values for hardware timer in Fan
#define Time1   10*60
#define Time2   20*60
#define Time3   30*60

// ----- Global functions -----
void ITHOinterrupt() IRAM_ATTR;
void ITHOcheck();

// ----- Global variables used in multiple files -----
typedef struct {
    String Id;
    String Roomname;
} IdDict;

extern IdDict Idlist[];
extern IthoCC1101 rf;
// extra for interrupt handling, set by itho-interrupt, read by itho_text_sensor
extern bool ITHOhasPacket;
// fan states
extern int State;
extern int OldState;
extern int Timer;
// ID identifying who changed the last state
extern String LastID;
extern String Mydeviceid;
