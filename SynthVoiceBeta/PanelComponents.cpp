#include "PanelComponents.h"

//---Switch------------------------------------------------------
PanelSwitch::PanelSwitch( void )
{

}

void PanelSwitch::init( uint8_t pinNum )
{
  pinNumber = pinNum;
  pinMode( pinNumber, INPUT_PULLUP );
  update();
  //force newData high in case knob starts on last value
  newData = 1;
}

void PanelSwitch::update( void )
{
  uint8_t tempState = digitalRead( pinNumber ) ^ 0x01;
  if( state != tempState )
  {
    state = tempState;
    newData = 1;
  }
}

uint8_t PanelSwitch::getState( void )
{
  newData = 0;

  return state;
}


//---Knob--------------------------------------------------------
PanelKnob8Bit::PanelKnob8Bit( void )
{

}

void PanelKnob8Bit::init( uint8_t pinNum )
{
  pinNumber = pinNum;
  pinMode( pinNumber, INPUT );
  update();
  //force newData high in case knob starts on last value
  newData = 1;
}

void PanelKnob8Bit::update( void )
{
  uint8_t tempState = analogRead( pinNumber ) >> 2;
  if( ( state > (tempState + 2) ) || ( state < (tempState - 2) ))
  {
    state = tempState;
    newData = 1;
  }
}

uint8_t PanelKnob8Bit::getState( void )
{
  newData = 0;

  return state;
}

//---Register----------------------------------------------------
PanelRegister::PanelRegister( void )
{

}

void PanelRegister::init( uint8_t pinNumBit2, uint8_t pinNumBit1, uint8_t pinNumBit0 )
{
  length = 3;
  pinMap[2] = pinNumBit2;
  pinMap[1] = pinNumBit1;
  pinMap[0] = pinNumBit0;

  pinMode( pinNumBit2, INPUT_PULLUP );
  pinMode( pinNumBit1, INPUT_PULLUP );
  pinMode( pinNumBit0, INPUT_PULLUP );

  update();
  //force newData high in case knob starts on last value
  newData = 1;
}

void PanelRegister::update( void )
{
  uint8_t tempState;
  tempState = digitalRead( pinMap[2] ) << 2;
  tempState |= digitalRead( pinMap[1] ) << 1;
  tempState |= digitalRead( pinMap[0] );

  //Invert
  tempState ^= 0x07;

  if( state != tempState )
  {
    state = tempState;
    newData = 1;
  }
}

uint8_t PanelRegister::getState( void )
{
  newData = 0;

  return state;
}


