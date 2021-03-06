//#include <avr/io.h>
//#include <avr/interrupt.h>
#include "wavegen.h"
#include "note_values.h"
#include <MIDI.h>
#include "Panel.h"

//Panel related variables
Panel myPanel;
float coarseTune = 0;
float fineTune = 0;
float dutyCycle = 0.5;
int8_t regCommand = -8;
uint8_t masterFunction = 7;
float rampVol = 1;
float sineVol = 1;
float pulseVol = 1;

uint8_t wavetable[256];
uint8_t wavetable_ptr;

uint8_t last_velo;
uint8_t last_volume = 20;

uint8_t last_key;
uint8_t voiceMidiChannel = 1;

//voice parameter variables:
uint16_t freq_whole;
uint16_t freq_partial;
uint16_t freq_partial_acu;

MIDI_CREATE_DEFAULT_INSTANCE();
int stat1led = 7;

// -----------------------------------------------------------------------------
void setup() 
{
  pinMode(stat1led, OUTPUT);
  //Generate table from libraries
  for(int i = 0; i < 256; i++)
  {
    wavetable[i] = (get_sample( SINESHAPE, 100, 100, i )) & 0x000000FF;
  }

  // initialize serial:
  //Serial.begin(9600);
  set_freq(100);
  //Serial.println(freq_whole);
  //Serial.println(freq_partial);


  //Enable pin 13 for LED
  pinMode(13, OUTPUT);
  //led
  pinMode( 6, OUTPUT );
  digitalWrite( 6, 1 );


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
}


void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  // Do whatever you want when a note is pressed.
  if( channel == voiceMidiChannel )
  {
    last_key = pitch;
    last_velo = velocity;

    set_freq(pitch);
    digitalWrite(stat1led, 0);
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
    digitalWrite(stat1led, 1);
  }

}

void handleControlChange(byte channel, byte number, byte value)
{
  if( number == 7 )
  {
    last_volume = value;
  }
  if( value < 16 )
  {
    digitalWrite(stat1led, 1);
  }
}

void loop()
{
  // Main code loop

    MIDI.read();

  myPanel.update();
  uint8_t gotShape = myPanel.ramp.newData | myPanel.sine.newData | myPanel.pulse.newData | myPanel.master.newData;
  //uint8_t gotRegister = myPanel.load.newData | myPanel.reg1.newData;
  //uint8_t gotKnob = myPanel.fine.newData | myPanel.coarse.newData ;

  if( gotShape )
  {
    digitalWrite( 6, 0 );  //Set LED
    //TIMSK1 = ~_BV(TOIE1);                                     //Stop int.
    cli();
    if( myPanel.master.newData )
    {
      myPanel.master.getState(); //dummy read
      if( masterFunction == 0 )
      {
        dutyCycle = (float)myPanel.master.getState() / 255;
      }
      if( masterFunction == 1 )
      {
        rampVol = (float)myPanel.master.getState() / 255;
      }
      if( masterFunction == 2 )
      {
        sineVol = (float)myPanel.master.getState() / 255;
      }
      if( masterFunction == 3 )
      {
        pulseVol = (float)myPanel.master.getState() / 255;
      }

    }

    int pulseOn = myPanel.pulse.getState();
    int sineOn = myPanel.sine.getState();
    int rampOn = myPanel.ramp.getState();

    int waveCalcTemp;
    int numWaveAdded;
    //Re-calculate shape
    for(int i = 0; i < 256; i++)
    {
      waveCalcTemp = 0;
      numWaveAdded = 0;
      if( rampOn )
      {
        waveCalcTemp += get_sample( RAMPSHAPE, rampVol, 1, i ) - 127;  //center
        numWaveAdded++;
      }
      if( sineOn )
      {
        waveCalcTemp += get_sample( SINESHAPE, sineVol, 1, i ) - 127;  //center
        numWaveAdded++;
      }
      if( pulseOn )
      {
        waveCalcTemp += get_sample( PULSESHAPE, pulseVol, dutyCycle, i ) - 127;  //center
        numWaveAdded++;
      }

      if( numWaveAdded > 0 )
      {
        wavetable[i] = ( waveCalcTemp / numWaveAdded ) + 127;  //127 justify
      }
      else
      {
        wavetable[i] = 127;
      }
    }
    sei();
    //GTCCR &= ~_BV(PSRSYNC);
    //TIMSK1 = _BV(TOIE1);                                     //Start int.
    digitalWrite( 6, 1 );  //Clear LED

  }
  if( myPanel.fine.newData )
  {
    //Process fine tune-- build the multiplier 2/3 to 4/3
    //f(x) = 2/3 * 1/255 * x + 2/3
    fineTune = myPanel.fine.getState() * .00261438 + 0.6666666;
    set_freq( last_key );

  }
  if( myPanel.coarse.newData )
  {
    //Process fine tune-- build the multiplier 0.25 to 4
    //f(x) = 7/2 * 1/255 * x + 0.5
    coarseTune = myPanel.coarse.getState() * .0137255 + 0.25;
    set_freq( last_key );

  }


  if( myPanel.load.newData && ( myPanel.load.getState() == 1) )
  {
    if( regCommand == -8 )
    {
      regCommand = myPanel.reg1.getState();
      digitalWrite( 6, 0 );  //Set LED

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
      default :
        break;
      }
      regCommand = -8;
      digitalWrite( 6, 1 );  //Clear LED
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
  math_temp = (note_frequency[note] * fineTune * coarseTune) * 256 / 80000;

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






