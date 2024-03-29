/*
 * BluetoothAudio_PassThru
 *
 * Created: Chip Audette, OpenAudio, Oct 2019
 * Purpose: Demonstrate basic audio pass-thru from the Bluetooth module
 *
 * Uses Teensy Audio default sample rate of 44100 Hz with Block Size of 128
 * Bluetooth Audio Requires Tympan Rev D
 *
 * License: MIT License, Use At Your Own Risk
 *
 */

//here are the libraries that we need
#include <Tympan_Library.h>  //include the Tympan Library

// define audio objects and connections
Tympan                        myTympan(TympanRev::E);  //do TympanRev::D or TympanRev::E
AudioInputI2S_F32         	  i2s_in;        //Digital audio *from* the Tympan AIC.
AudioOutputI2S_F32        	  i2s_out;       //Digital audio *to* the Tympan AIC.  Always list last to minimize latency

AudioConnection_F32           patchCord1(i2s_in, 0, i2s_out, 0); //connect left input to left output
AudioConnection_F32           patchCord2(i2s_in, 1, i2s_out, 1); //connect right input to right output

// define the setup() function, the function that is called once when the device is booting
void setup(void)
{
  //begin the serial comms (for debugging)
  Serial.begin(115200);  delay(500);
  Serial.println("BluetoothAudio_PassThru: Starting setup()...");

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(10); 

  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate AIC

  //Choose the desired input
  if (1) {
    
    //Configure the Tympan to digitize the analog audio produced by
    //the Bluetooth module 
    myTympan.inputSelect(TYMPAN_INPUT_BT_AUDIO); 
    myTympan.setInputGain_dB(0.0); // keep the BT audio at 0 dB gain
    
  } else {
    
    //Or, digitize audio from one of the other sources and simply mix
    //the bluetooth audio via the analog output mixer built into system.

    //select which channel to digitize
    myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
    //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
    //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

    //set the input gain for this channel
    myTympan.setInputGain_dB(15.0);                      // Set input gain.  0-47.5dB in 0.5dB setps

    //now, on the output, mix in the analog audio from the BT module
    myTympan.mixBTAudioWithOutput(true);                 // this tells the tympan to mix the BTAudio into the output
  }

  //Set the desired output volume levels
  myTympan.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.

  Serial.println("Setup complete.");
}

void loop(void)
{
  // Nothing to do - just looping input to output
  delay(2000);
  Serial.println("Running...");
}
