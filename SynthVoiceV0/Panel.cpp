#include "Panel.h"

#define loadPin 2
#define regBit2Pin 3
#define regBit1Pin 4
#define regBit0Pin 5
#define rampPin 8
#define sinePin 9
#define pulsePin 10
#define finePin A1
#define coarsePin A2
#define masterPin A0
#define stat1LedPin 6
#define stat2LedPin 7

Panel::Panel( void )
{

}

void Panel::init( void )
{
  ramp.init( rampPin );
  sine.init( sinePin );
  pulse.init( pulsePin );
  load.init( loadPin );
  fine.init( finePin );
  coarse.init( coarsePin );
  master.init( masterPin );
  reg1.init( regBit2Pin, regBit1Pin, regBit0Pin );
  stat1Led.init(stat1LedPin, 0);
  stat2Led.init(stat2LedPin, 0);
  
}

void Panel::update( void )
{
  ramp.update();
  sine.update();
  pulse.update();
  load.update();
  fine.update();
  coarse.update();
  master.update();
  reg1.update();
  stat1Led.update();
  stat2Led.update();
  
}

