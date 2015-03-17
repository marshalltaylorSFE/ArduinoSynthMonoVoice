#ifndef PANELCOMPONENTS_H
#define PANELCOMPONENTS_H
#include <Arduino.h>

//---Switch------------------------------------------------------
class PanelSwitch
{
public:
  PanelSwitch( void );
  void update( void );
  void init( uint8_t );
  uint8_t getState( void );
  uint8_t state;
  uint8_t invert;
  uint8_t pinNumber;
  uint8_t newData;
protected:
private:
};

//---Knob--------------------------------------------------------
class PanelKnob8Bit
{
public:
  PanelKnob8Bit( void );
  void update( void );
  void init( uint8_t );
  uint8_t getState( void );
  uint8_t state;
  uint8_t pinNumber;
  uint8_t newData;
protected:
private:
};

//---Register----------------------------------------------------
class PanelRegister
{
public:
  PanelRegister( void );
  void update( void );
  void init( uint8_t, uint8_t, uint8_t );
  uint8_t getState( void );
  uint8_t state;
  uint8_t pinMap[8];
  uint8_t length;
  uint8_t newData;
protected:
private:
  PanelSwitch switchBit0;
  PanelSwitch switchBit1;
  PanelSwitch switchBit2;
};

#endif // PANELCOMPONENTS_H


