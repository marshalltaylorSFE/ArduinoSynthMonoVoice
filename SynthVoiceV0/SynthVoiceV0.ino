//#include <avr/io.h>
//#include <avr/interrupt.h>
#include "wavegen.h"
#include "note_values.h"
#include <MIDI.h>
#include "flagMessaging.h"
#include "PanelComponents.h"
#include "Panel.h"
#include "Wire.h"

//Panel related variables
Panel myPanel;
float coarseTune = 1;
float fineTune = 1;
uint8_t dutyCycle = 128;
int8_t regCommand = -8;
uint8_t masterFunction = 0;
uint8_t rampVol = 255;
uint8_t sineVol = 255;
uint8_t pulseVol = 255;
uint8_t masterVol = 255;
int8_t octaveAdjust = 0;
uint8_t protectTuning = 1;

uint8_t wavetable[256];
uint8_t wavetable_ptr;

uint8_t last_velo = 0;
uint8_t last_volume = 0;

uint8_t last_key = 24;
uint8_t voiceMidiChannel = 1;

//voice parameter variables:
uint16_t freq_whole;
uint16_t freq_partial;
uint16_t freq_partial_acu;

MIDI_CREATE_DEFAULT_INSTANCE();

// -----------------------------------------------------------------------------
void setup() 
{
  //Generate table from libraries
  for(int i = 0; i < 256; i++)
  {
    wavetable[i] = 128;
  }

  // initialize serial:
  //Serial.begin(9600);
  set_freq(100);
  //Serial.println(freq_whole);
  //Serial.println(freq_partial);

  myPanel.init();

  //Start up timer1 on no pins
  TCCR1A = 0;
  TCCR1B = _BV(WGM13);// | _BV(CS10);
  ICR1 = 0;//This is the reg to control the sample rate
  ICR1 = 100;//This is the reg to control the sample rate
  TCCR1B &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12));
  TCCR1B |= _BV(CS10);  

  //Start up timer2 on pin 11
  pinMode(11, OUTPUT);
  TCCR2A = _BV(COM2A1) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(CS20);
  OCR2A = 1;//This is the reg to write to to control PWM
  GTCCR &= ~_BV(PSRSYNC);
  TIMSK1 = _BV(TOIE1);                                     // sets the timer overflow interrupt enable bit
  //Timer1.attachInterrupt( timerIsr ); // attach the service routine here

  // so it is called upon reception of a NoteOn.
  MIDI.setHandleNoteOn(handleNoteOn);  // Put only the name of the function

    // Do the same for NoteOffs
  MIDI.setHandleNoteOff(handleNoteOff);

  // For control
  MIDI.setHandleControlChange(handleControlChange);

  MIDI.begin(MIDI_CHANNEL_OMNI);
  
  //Used for debug
  //myDebugger.beginOutputs(4, 5);//Clock, data
  
}


void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  // Do whatever you want when a note is pressed.
  if( channel == voiceMidiChannel )
  {
    last_key = pitch;
    last_velo = velocity;

    set_freq(pitch);
	myPanel.stat1Led.setState(LEDON);
  }
  //Serial.println(pitch);

  // Try to keep your callbacks short (no delays ect)
  // otherwise it would slow down the loop() and have a bad impact
  // on real-time performance.
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  if( channel == voiceMidiChannel )
  {
    if( pitch == last_key )
    {
      //turn off, or in this case make very high frequency
      //set_freq(127);
    }
    else //a new key has been pressed
    {
      //do nothing
    }
    myPanel.stat1Led.setState(LEDOFF);
  }

}

void handleControlChange(byte channel, byte number, byte value)
{
  if( number == 7 )
  {
    last_volume = value << 1;
  }
  if( value < 16 )
  {
    //myPanel.stat1Led.setState(LEDOFF);
  }
}

void loop()
{
  // Main code loop

    MIDI.read();

	myPanel.sine.SwitchDebounceTimeKeeper.mIncrement(1);
	myPanel.pulse.SwitchDebounceTimeKeeper.mIncrement(1);
	myPanel.ramp.SwitchDebounceTimeKeeper.mIncrement(1);
	myPanel.load.SwitchDebounceTimeKeeper.mIncrement(1);
  
  myPanel.update();
  
  uint8_t gotShape = 
    myPanel.sine.state.serviceRisingEdge()
  | myPanel.sine.state.serviceFallingEdge()
  | myPanel.pulse.state.serviceRisingEdge()
  | myPanel.pulse.state.serviceFallingEdge()
  | myPanel.ramp.state.serviceRisingEdge()
  | myPanel.ramp.state.serviceFallingEdge()
  | myPanel.master.newData;
  //uint8_t gotRegister = myPanel.load.newData | myPanel.reg1.newData;
  //uint8_t gotKnob = myPanel.fine.newData | myPanel.coarse.newData ;

  if( gotShape )
  {
    myPanel.stat2Led.setState(LEDON);
	myPanel.stat2Led.update();
    //TIMSK1 = ~_BV(TOIE1); //Stop int.
    cli();
    if( myPanel.master.newData )
    {
      myPanel.master.getState(); //dummy read
      if( masterFunction == 4 )
      {
        dutyCycle = myPanel.master.getState();
      }
      if( masterFunction == 1 )
      {
        rampVol = myPanel.master.getState();
      }
      if( masterFunction == 2 )
      {
        sineVol = myPanel.master.getState();
      }
      if( masterFunction == 3 )
      {
        pulseVol = myPanel.master.getState();
      }
      if( masterFunction == 0 )  //Default state
      {
        masterVol = myPanel.master.getState();
      }
    }

    //Re-calculate shape
    
    WaveGenerator testWave;
    testWave.resetOffset();
    // Parameters (all uint8_t): ( master, ramp, sine, pulseAmp, pulseDuty )
    testWave.setParameters( masterVol, myPanel.ramp.state.getFlag() * rampVol, myPanel.sine.state.getFlag() * sineVol, myPanel.pulse.state.getFlag() * pulseVol, dutyCycle );
    for(int i = 0; i < 256; i++)
    {
      wavetable[i] = testWave.getSample();

    }
    sei();
    //GTCCR &= ~_BV(PSRSYNC);
    //TIMSK1 = _BV(TOIE1);                                     //Start int.
    myPanel.stat2Led.setState(LEDOFF);  //Clear LED
	myPanel.stat2Led.update();

  }
  if( myPanel.fine.newData && !protectTuning )
  {
    //Process fine tune-- build the multiplier 2/3 to 4/3
    //f(x) = 2/3 * 1/255 * x + 2/3
    fineTune = myPanel.fine.getState() * .00261438 + 0.6666666;
    set_freq( last_key );

  }
  if( myPanel.coarse.newData && !protectTuning )
  {
    //Process fine tune-- build the multiplier 0.25 to 4
    //f(x) = 7/2 * 1/255 * x + 0.5
    coarseTune = myPanel.coarse.getState() * .0137255 + 0.25;
    set_freq( last_key );

  }


  if( myPanel.load.state.serviceRisingEdge() == 1 )  //If load switched and is equal to 1
  {
    if( regCommand == -8 )
    {
      regCommand = myPanel.reg1.getState();  //save the reg to command
      myPanel.stat2Led.setState(LEDON);  //Set LED
	  myPanel.stat2Led.update();

    }
    else
    {
      switch( regCommand )  //Process last input command
      {
      case 0x07 :
        //set midi channel;
        voiceMidiChannel = myPanel.reg1.getState() + 1;
        break;
      case 0x00 :
        //set master knob state
        masterFunction = myPanel.reg1.getState();
        break;
      case 0x01 :
        if( myPanel.reg1.getState() == 0 )
        {
          //tuning centered
          protectTuning = 1;
          coarseTune = 1;
          fineTune = 1;
          set_freq( last_key );
        }
        else
        {
          protectTuning = 0;
        }
        break;
      case 0x02 :
        //set octave
        octaveAdjust = myPanel.reg1.getState();
        if( octaveAdjust > 3 )  //kind of a forced cast
        {
          //convert to negitive
          octaveAdjust = octaveAdjust - 8;
        }
        set_freq( last_key );
        break;
      default :
        break;
      }
      regCommand = -8;
      myPanel.stat2Led.setState(LEDOFF);  //Clear LED
	  myPanel.stat2Led.update();
    }


  }
}

/// --------------------------
/// Custom ISR Timer Routine
/// --------------------------
ISR(TIMER1_OVF_vect)
{
  //add the whole part
  wavetable_ptr += freq_whole;

  //add the partial to the accumulator
  freq_partial_acu += freq_partial;

  //if the partial is greater than 1, crop the accumulator and increment the pointer
  wavetable_ptr += freq_partial_acu >> 15;
  freq_partial_acu &= 0x7FFF;

  wavetable_ptr &= 0x00FF;
  //scaling
  //output = wavesample * volume/127 + 127 - volume
  OCR2A = ((wavetable[wavetable_ptr] * last_volume) >> 7) + (127 - last_volume);

}

//This function generates the whole and partial values, and PUTS THEM IN THE GLOBAL CONTEXT
//  Also, there's no blocking between commands
//
//
void set_freq( char note )  // Give the midi note number
{
  float math_temp;
  int16_t noteAdjust = note + octaveAdjust * 12;
  if( noteAdjust < 1 )
  {
    noteAdjust = 0;
  }
  if( noteAdjust > 127 )
  {
    noteAdjust = 127;
  }
  math_temp = (note_frequency[noteAdjust] * fineTune * coarseTune) * 256 / 80000;
  
  //End tuning
  freq_whole = (uint16_t)math_temp;  //This can be unloaded into note_values.c if necessary
  freq_partial = (uint16_t)(( math_temp - freq_whole ) * 32768);
  return;
}




// This takes a int input and converts to char.  This replaces the shitty type conversion of Serial.write()
//
// The output will be an char if the int
//  is between zero and 0xF
//
// Otherwise, the output is $.
//
char hex2char(int hexin)
{
  int charout;
  charout = 0x24; // default $ state
  if(hexin >= 0x00)
  {
    if(hexin <= 0x09)
    {
      charout = hexin + 0x30;
    }
  }
  if(hexin >= 0x0A)
  {
    if(hexin <= 0x0F)
    {
      charout = hexin -10 + 0x41;
    }
  }  
  return charout;
}


void sendDebugString( char* stringInput )
{
  uint8_t i = 0;
  while( stringInput[i] != 0x00 )
  {
    //myDebugger.sendByte( stringInput[i] );
    delay(20);
    i++;
  }

}




