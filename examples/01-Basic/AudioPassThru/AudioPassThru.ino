/*
 * AudioPassThru
 *
 * Created: Chip Audette, OpenAudio, Jan/Feb 2017
 * Purpose: Demonstrate basic audio pass-thru using the Tympan Audio Board,
 *     which is based on the TI TLV320AIC3206
 *
 * Uses Teensy Audio default sample rate of 44100 Hz with Block Size of 128
 *
 * License: MIT License, Use At Your Own Risk
 *
 */

//here are the libraries that we need
#include <Audio.h>           //include the Teensy Audio Library
#include <Tympan_Library.h>  //include the Tympan Library

// define audio objects and connections
AudioControlTLV320AIC3206     audioHardware;
AudioInputI2S_F32         	  i2s_in;        //Digital audio *from* the Tympan AIC.
AudioOutputI2S_F32        	  i2s_out;       //Digital audio *to* the Tympan AIC.  Always list last to minimize latency

AudioConnection_F32           patchCord1(i2s_in, 0, i2s_out, 0); //connect left input to left output
AudioConnection_F32           patchCord2(i2s_in, 1, i2s_out, 1); //connect right input to right output

// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 20.0f; //gain on the microphone
void setup(void)
{
  //begin the serial comms (for debugging)
  Serial.begin(115200);  delay(500);
  Serial.println("AudioPassThru: Starting setup()...");

  //allocate the audio memory
  AudioMemory(10); AudioMemory_F32(10); //allocate both kinds of memory

  //Enable the Tympan to start the audio flowing!
  audioHardware.enable(); // activate AIC

  //Choose the desired input
  audioHardware.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on board microphones
  // audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack - defaults to mic bias 2.5V
  //  audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  audioHardware.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  audioHardware.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  Serial.println("Setup complete.");
}

void loop(void)
{
  // Nothing to do - just looping input to output
  delay(2000);
  Serial.println("Running...");
}
