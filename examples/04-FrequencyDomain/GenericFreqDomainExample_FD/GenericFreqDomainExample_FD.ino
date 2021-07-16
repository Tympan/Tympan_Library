// GenericFreqDomainExample_FD
//
// Demonstrate audio procesing in the frequency domain using the Tympan_Library's base class
// that encapsulates much of the detailed work of converting from the time domain to the 
// frequency domain and back.
//
// Created: Chip Audette, OpenAudio, June 2021
//
// Approach:
//    * Take samples in the time domain
//    * Take FFT to convert to frequency domain
//    * Manipulate the frequency bins as desired (LP filter?  BP filter?  Formant shift?)
//    * Take IFFT to convert back to time domain
//    * Send samples back to the audio interface
//
// This example code is in the public domain (MIT License)

#include <Tympan_Library.h>
#include "AudioEffectLowpass_FD_F32.h"  //the local file holding your custom function

//set the sample rate and block size
const float sample_rate_Hz = 24000.f; ; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 32;     //for freq domain processing choose a power of 2 (16, 32, 64, 128) but no higher than 128
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
Tympan                       myTympan(TympanRev::E);                //do TympanRev::D or TympanRev::E
AudioInputI2S_F32            i2s_in(audio_settings);                //Digital audio *from* the Tympan AIC.
AudioEffectLowpass_FD_F32    audioEffectLowpassFD(audio_settings);  //create an example frequency-domain processing block
AudioOutputI2S_F32           i2s_out(audio_settings);               //Digital audio *to* the Tympan AIC.

//Make all of the audio connections
AudioConnection_F32       patchCord1(i2s_in, 0, audioEffectLowpassFD, 0);   //connect the Left input to our algorithm
AudioConnection_F32       patchCord2(audioEffectLowpassFD, 0, i2s_out, 0);  //connect the algorithm to the left output
AudioConnection_F32       patchCord3(audioEffectLowpassFD, 0, i2s_out, 1);  //connect the algorithm to the right output

// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 15.0f; //gain on the microphone
float vol_knob_gain_dB = 0.0;      //will be overridden by volume knob
void setup() {
  //begin the serial comms (for debugging)
  myTympan.beginBothSerial();delay(3000);
  Serial.println("GenericFrequencyProcessing_FD: starting setup()...");
  Serial.print("    : sample rate (Hz) = ");  Serial.println(audio_settings.sample_rate_Hz);
  Serial.print("    : block size (samples) = ");  Serial.println(audio_settings.audio_block_samples);

  // Allocate working memory for audio
  AudioMemory_F32(20, audio_settings);

  // Configure the frequency-domain algorithm
  int N_FFT = 128;
  audioEffectLowpassFD.setup(audio_settings,N_FFT); //do after AudioMemory_F32();
  audioEffectLowpassFD.setCutoff_Hz(1000.0);
  audioEffectLowpassFD.setHighFreqGain_dB(-20.0);
  

 //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate AIC

  //Choose the desired input
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on board microphones
  // myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack - defaults to mic bias 2.5V
  // myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  myTympan.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  myTympan.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  // configure the blue potentiometer
  servicePotentiometer(millis(),0);  //update based on the knob setting the "0" is not relevant here.

  Serial.println("Setup complete.");
}


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //check the potentiometer
  servicePotentiometer(millis(),100); //service the potentiometer every 100 msec

  //check to see whether to print the CPU and Memory Usage
  myTympan.printCPUandMemory(millis(),3000); //print every 3000 msec

} //end loop();


// ///////////////// Servicing routines

//servicePotentiometer: listens to the blue potentiometer and sends the new pot value
//  to the audio processing algorithm as a control parameter
void servicePotentiometer(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 100; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;
  static float prev_val = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    //read potentiometer
    float val = float(myTympan.readPotentiometer()) / 1023.0; //0.0 to 1.0
    val = (1.0/15.0) * (float)((int)(15.0 * val + 0.5)); //quantize so that it doesn't chatter...0 to 1.0

    //send the potentiometer value to your algorithm as a control parameter
    if (abs(val - prev_val) > 0.05) { //is it different than before?
      prev_val = val;  //save the value for comparison for the next time around

      #if 0
        //use the potentiometer as a volume knob
        const float min_val = 0, max_val = 40.0; //set desired range
        float new_value = min_val + (max_val - min_val)*val;
        input_gain_dB = new_value;
        myTympan.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps
        Serial.print("servicePotentiometer: Input Gain (dB) = "); Serial.println(new_value); //print text to Serial port for debugging
      #else
        //use the potentiometer to set the freq-domain low-pass filter
        const float min_val = logf(200.f), max_val = logf(12000.f); //set desired range
        float lowpass_Hz = expf(min_val + (max_val - min_val)*val);
        audioEffectLowpassFD.setCutoff_Hz(lowpass_Hz);
        Serial.print("servicePotentiometer: Lowpass (Hz) = "); Serial.println(lowpass_Hz); //print text to Serial port for debugging
      #endif
    }
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();
