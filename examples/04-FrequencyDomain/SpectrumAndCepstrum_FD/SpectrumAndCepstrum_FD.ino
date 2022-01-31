// SpectrumAndCepstrum_FD
//
// Demonstrate doing spectral and cepstral analysis.  
//
// Created: Chip Audette, OpenAudio, Nov 2021
//
// Approach:
//    * Take samples in the time domain
//    * Take FFT to convert to frequency domain (spectrum)
//    * Magnitude and log
//    * Take FFT again (cepstrum)
//    * Make spectrum and cepstrum data available
//
// Use the Arduino SerialPlotter!!!!  Sing SLOWLY into your Tympan!  Watch the spectrum and cepstrum!
//
// This example code is in the public domain (MIT License)

#define OUTPUT_FOR_SERIAL_PLOTTER true    //set to true and then open the Arduino SerialPlotter rather than the Serial Monitor

#include <Tympan_Library.h>
#include "AudioAnalysisCepstrum_FD_F32.h"  //the local file holding your custom function

//set the sample rate and block size
const float sample_rate_Hz = 16000.f; //try diff values, like 16000, 24000, 32000, 44100, 48000 or 96000...slower gives more frequency resolution
const int audio_block_samples = 128;  //choose a power of 2 (16, 32, 64, 128) but no higher than 128
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
Tympan                         myTympan(TympanRev::E,audio_settings);  //do TympanRev::D or TympanRev::E
AudioInputI2S_F32              i2s_in(audio_settings);                 //Digital audio *from* the Tympan AIC.
AudioAnalysisCepstrum_FD_F32   audioAnalysisCepstrum(audio_settings);  //computes spectrum and cepstrum
AudioOutputI2S_F32             i2s_out(audio_settings);                //Digital audio *to* the Tympan AIC.

//Make all of the audio connections
AudioConnection_F32       patchCord1(i2s_in, 0, audioAnalysisCepstrum, 0);   //connect the Left input to our algorithm
AudioConnection_F32       patchCord2(i2s_in, 0, i2s_out, 0);  //connect the algorithm to the left output
AudioConnection_F32       patchCord3(i2s_in, 0, i2s_out, 1);  //connect the algorithm to the right output

//add our printing functions
#include "printResults.h"  //here is a local file holding the spectral/cepstral printing functions


// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 15.0f; //gain on the microphone
float vol_knob_gain_dB = 0.0;      //will be overridden by volume knob
void setup() {
  //begin the serial comms (for debugging)
  myTympan.beginBothSerial();
  while ( (!Serial) && (millis() < 1000));  //stall for 1000 msec in case USB is attached at startup
  #if (OUTPUT_FOR_SERIAL_PLOTTER == false)
    Serial.println("SpectrumAndCepstrum_FD: starting setup()...");
    Serial.print("    : sample rate (Hz) = ");  Serial.println(audio_settings.sample_rate_Hz);
    Serial.print("    : block size (samples) = ");  Serial.println(audio_settings.audio_block_samples);
  #endif

  // Allocate working memory for audio
  AudioMemory_F32(20, audio_settings);

  // Configure the frequency-domain algorithm
  int N_FFT = 8*audio_block_samples;  //choose no more than 8 x audio_block_samples?  Maybe it'll work at 8x or higher?  Maybe not?  Try it!
  #if (OUTPUT_FOR_SERIAL_PLOTTER == false)
    Serial.println("setup: Setting to N_FFT = " + String(N_FFT));
  #endif
  audioAnalysisCepstrum.setup(audio_settings,N_FFT); //do after AudioMemory_F32();
  
 //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate AIC

  //setup DC-blocking highpass filter running in the ADC hardware itself
  float cutoff_Hz = 40.0;  //set the default cutoff frequency for the highpass filter
  myTympan.setHPFonADC(true,cutoff_Hz,audio_settings.sample_rate_Hz); //set to false to disble

  //Choose the desired input
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on board microphones
  // myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack - defaults to mic bias 2.5V
  // myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  myTympan.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  myTympan.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  #if (OUTPUT_FOR_SERIAL_PLOTTER == false)
    Serial.println("Setup complete.");
  #endif
}


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  #if (OUTPUT_FOR_SERIAL_PLOTTER == false)
  
    //print out a handful of speak spectral and cepstral data points
    printSpectralAndCepstralPeaks(millis(),1000); //print every 1000 msec

    //check to see whether to print the CPU and Memory Usage
    myTympan.printCPUandMemory(millis(),3000); //print every 3000 msec
  
  #else

    //print out full spectrum and cepstrum...for serial plotter!
    printFullSpectrumAndCepstrum(millis(),500);  //print every 500 msec
  
  #endif
  


} //end loop();
