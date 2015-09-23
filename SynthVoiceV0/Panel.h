#ifndef PANEL_H
#define PANEL_H

#include "PanelComponents.h"
#include "flagMessaging.h"
#include <Arduino.h>


class Panel
{
public:
  Panel( void );
  void update( void );
  void init( void );
  PanelSwitch ramp;
  PanelSwitch pulse;
  PanelSwitch sine;
  PanelSwitch load;
  PanelKnob8Bit fine;
  PanelKnob8Bit coarse;
  PanelKnob8Bit master;
  PanelRegister reg1;
  PanelLed stat1Led;
  PanelLed stat2Led;
  //PanelRegister switch1( registerB2Pin, registerB1Pin, registerB0Pin );
  //PanelKnob master( masterPin );
  //PanelKnob coarse( coarsePin );
  //PanelKnob fine( finePin );
protected:
private:
};

#endif // PANEL_H



