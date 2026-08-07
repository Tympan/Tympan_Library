// Resampling_FD
//
// Demonstrate audio procesing where data is acquired at one sample rate, downsampled via
// frequency-domain processing (an expensive way to do it), lowpass filtered in the time domain,
// and then upsampled to return back to the original sample rate.
//
// The Tympan hardware requires that the sample rate requested for the input (from i2s_in)
// must be the same sample rate used for the output (to i2s_out).  In this sketch, we
// adhere to that requirement while giving ourselves flexibility to run a different
// sample rate in the middle.
//
// This example runs at 96 kHz.  Without the resampling (ie dcimation_factor = 1), the CPU
// usage is about 9.3% on a Tympan Rev F.  But, using decimation_factor = 4, the intermediate
// processing is performed at 96000/4 = 24000, which lowers the CPU usage to 6.1%.  If you
// were doing more intermediate processing (more than just a lowpass filter), you would see
// a greater difference between the raw and decimated CPU values.  [On a Tympan RevD at 96 kHz,
// it's 67% CPU with decimation 1, but it's only 42% CPU with decimation = 4]
//
// Of course, the main reason to run at high sample rates is to get access to the higher
// frequencies, which the decimation defeats.  But, if you have a reason why you must
// sample at a higher sample rate, but you have some portion of your processing that 
// can be done at a lower sample rate, this example might help you.
//
// Created: Chip Audette, OpenAudio, Aug 2026
//
// This example code is in the public domain (MIT License)

#include <Tympan_Library.h>
#include "AudioEffectResample_FD_F32_local.h"  //the local file holding your custom function

// //////////////////////////////////////////////////// IMPORTANT
// Here's where we choose our different sampling rates.

//set the sample rate and block size to/from the hardware
const float original_sample_rate_Hz = 96000.f;
const int original_audio_block_samples = 128;     //for freq domain processing choose a power of 2 (16, 32, 64, 128) but no higher than 128
AudioSettings_F32 original_audio_settings(original_sample_rate_Hz, original_audio_block_samples);

// set the reduced sample rate to use for intermediate calculationsc
const int decimation_factor = 1;    // this is our choice.  By how much do we want to decimate the sampele rate?  Choose any power of 2 (the FFTs might not work right otherwise)
const float downsampled_sample_rate_Hz = original_sample_rate_Hz / decimation_factor;
const int downsampled_audio_block_samples = original_audio_block_samples / decimation_factor;     //for freq domain processing choose a power of 2 (16, 32, 64, 128) but no higher than 128
AudioSettings_F32 downsampled_audio_settings(downsampled_sample_rate_Hz, downsampled_audio_block_samples);

//create oject representing the Tympan itself
Tympan                      myTympan(TympanRev::D, original_audio_settings);  //use the audio_settings describing what the AIC hardware is being asked to do

// create the audio library objects carefully choosing the correction audio_settings for each
AudioInputI2S_F32                i2s_in(     original_audio_settings);     //Digital audio *from* the Tympan AIC hardware.
AudioEffectResample_FD_F32_local downsample( original_audio_settings);     //use the audio_settings for the audio coming *into* the object (but doesn't really matter as we'll re-initialize via setup() later)  
AudioFilterBiquad_F32            lpFilt(     downsampled_audio_settings);  //the audio is all at the downsampled rate now
AudioEffectResample_FD_F32_local upsample(   downsampled_audio_settings);  //use the audio_settings for the audio coming *into* the object (but doesn't really matter as we'll re-initialize via setup() later)  
AudioOutputI2S_F32               i2s_out(    original_audio_settings);     //Digital audio *to* the Tympan AIC hardware.

//Make all of the audio connections
AudioConnection_F32       patchCord1(i2s_in, 0, downsample, 0);          // get the left input and downsample
AudioConnection_F32       patchCord2(downsample, 0, lpFilt, 0);   // pass the downsampled data to our filter
AudioConnection_F32       patchCord3(lpFilt, 0, upsample, 0);     // filtered output to get upsampled back to the original rate
AudioConnection_F32       patchCord4(upsample, 0, i2s_out, 0);           //connect the algorithm to the left output
AudioConnection_F32       patchCord5(upsample, 0, i2s_out, 1);           //connect the algorithm to the right output

// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 15.0f; //gain on the microphone
float vol_knob_gain_dB = 0.0;      //will be overridden by volume knob
float cutoff_Hz = 1000.0;
void setup() {
  //begin the serial comms (for debugging)
  myTympan.beginBothSerial();delay(3000);
  Serial.println("Resampling_FD: starting setup()...");
  Serial.print("    : original sample rate (Hz) = ");        Serial.println(original_audio_settings.sample_rate_Hz);
  Serial.print("    : original block size (samples) = ");    Serial.println(original_audio_settings.audio_block_samples);
  Serial.print("    : downsampled sample rate (Hz) = ");     Serial.println(downsampled_audio_settings.sample_rate_Hz);
  Serial.print("    : downsampled block size (samples) = "); Serial.println(downsampled_audio_settings.audio_block_samples);

  // Allocate working memory for audio
  AudioMemory_F32(20, original_audio_settings);

  // setup the resampling
  const int NFFT_orig = 2*original_audio_block_samples; // use 1, 2, or 4 times the block size
  const int NFFT_downsampled = NFFT_orig / decimation_factor;
  downsample.setup(original_audio_settings, NFFT_orig, NFFT_downsampled);
  upsample.setup(downsampled_audio_settings, NFFT_downsampled, NFFT_orig);

  // set the other algorithms
  lpFilt.setLowpass(0,cutoff_Hz);

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
  static float prev_val = 0.0;

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
        const float min_val = 0.0, max_val = 40.0; //set desired range
        float new_value = min_val + (max_val - min_val)*val;
        input_gain_dB = new_value;
        myTympan.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps
        Serial.print("servicePotentiometer: Input Gain (dB) = "); Serial.println(new_value); //print text to Serial port for debugging
      #else
        //use the potentiometer to set the freq-domain low-pass filter
        const float min_val = logf(200.f), max_val = logf(min(12000.f,0.95*downsampled_sample_rate_Hz/2.0f)); //set desired range
        float lowpass_Hz = expf(min_val + (max_val - min_val)*val);
        lpFilt.setLowpass(0,lowpass_Hz);
        Serial.print("servicePotentiometer: Lowpass (Hz) = "); Serial.println(lowpass_Hz); //print text to Serial port for debugging
      #endif
    }
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();
