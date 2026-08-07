// Resampling
//
// Demonstrate audio procesing where data is acquired at one sample rate, downsampled (via
// time-domain processing), run through WDRC compressor, and then upsampled (in the time 
// domain) to return back to the original sample rate.
//
// The Tympan hardware requires that the sample rate requested for the input (from i2s_in)
// must be the same sample rate used for the output (to i2s_out).  In this sketch, we
// adhere to that requirement while giving ourselves flexibility to run a different
// sample rate in the middle.
//
// This example runs at 96 kHz, which can then be downsampled (via "decimation_factor") to
// lower the sample rate for the application of a WDRC compressor.  The table below shows
// the CPU savings.  If you were doing more intermediate processing (more than just this one
// compressor), you would see a greater difference between the raw and decimated CPU values.
//
// CPU Results for Primary Sample Rate = 96000
//
//   Tympan Rev F, Load:  one AudioEffectCompWDRC_F32 compressor
//     no decimation, just the compressor at the full 96 kHz:       CPU = 4.5%
//     decimation fac = 1, N_dec_filter=20, N_interp_filter=linear: CPU = 4.5% [no penalty or benefit vs original]
//     decimation fac = 4, N_dec_filter=20, N_interp_filter=linear: CPU = 2.5%  [2.0% savings vs original]
//
//   Tympan Rev D, Load: one AudioEffectCompWDRC_F32 compressor
//     no decimation, just the compressor at the full 96 kHz:       CPU = 14.7%
//     decimation fac = 1, N_dec_filter=20, N_interp_filter=linear: CPU = 16.5% [1.8% penalty vs original]
//     decimation fac = 4, N_dec_filter=20, N_interp_filter=linear: CPU = 9.9%  [4.8% savings vs original]
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

// //////////////////////////////////////////////////// IMPORTANT: SETTING THE SAMPLE RATES
// Here's where we choose our different sampling rates.

//set the sample rate and block size to/from the hardware
constexpr float original_sample_rate_Hz = 96000.f;
constexpr int original_audio_block_samples = 128;     //for freq domain processing choose a power of 2 (16, 32, 64, 128) but no higher than 128
AudioSettings_F32 original_audio_settings(original_sample_rate_Hz, original_audio_block_samples);

// set the reduced sample rate to use for intermediate calculationsc
constexpr unsigned int decimation_factor = 4;    // this is our choice.  By how much do we want to decimate the sample rate?  Choose any integer value
constexpr float downsampled_sample_rate_Hz = original_sample_rate_Hz / decimation_factor;
constexpr int downsampled_audio_block_samples = original_audio_block_samples / decimation_factor;     //for freq domain processing choose a power of 2 (16, 32, 64, 128) but no higher than 128
AudioSettings_F32 downsampled_audio_settings(downsampled_sample_rate_Hz, downsampled_audio_block_samples);

// ////////////////////////////////////////////// CONTINUING WITH NORMAL SETUP...

//create oject representing the Tympan itself
Tympan                      myTympan(TympanRev::F, original_audio_settings);  //use the audio_settings describing what the AIC hardware is being asked to do

// create the audio library objects carefully choosing the correction audio_settings for each
AudioInputI2S_F32           i2s_in(     original_audio_settings);     //Digital audio *from* the Tympan AIC hardware.
AudioRateDecimator_F32      downsample( original_audio_settings);     //use the audio_settings for the audio coming *into* the object (but doesn't really matter as we'll re-initialize via setup() later)
AudioEffectCompWDRC_F32     comp1(      downsampled_audio_settings);  //processing that uses a good amount of CPU
AudioRateInterpolator_F32   upsample(   downsampled_audio_settings);  //use the audio_settings for the audio coming *into* the object (but doesn't really matter as we'll re-initialize via setup() later)  
AudioOutputI2S_F32          i2s_out(    original_audio_settings);     //Digital audio *to* the Tympan AIC hardware.

//Make all of the audio connections
#if 1
  //use the decimation/interpolation
  AudioConnection_F32       patchCord1(i2s_in, 0, downsample, 0);       // get the left input and downsample
  AudioConnection_F32       patchCord2(downsample, 0, comp1, 0);      // pass the downsampled data to our filter
  AudioConnection_F32       patchCord5(comp1, 0, upsample, 0);        // filtered output to get upsampled back to the original rate
  AudioConnection_F32       patchCord6(upsample, 0, i2s_out, 0);        //connect the algorithm to the left output
  AudioConnection_F32       patchCord7(upsample, 0, i2s_out, 1);        //connect the algorithm to the right output
#else
  //do not use the decimation/interpolation
  AudioConnection_F32       patchCord2(i2s_in, 0, comp1, 0);          // pass the downsampled data to our filter
  AudioConnection_F32       patchCord6(comp1, 0, i2s_out, 0);         //connect the algorithm to the left output
  AudioConnection_F32       patchCord7(comp1, 0, i2s_out, 1);         //connect the algorithm to the right output
#endif


// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 15.0f; //gain on the microphone
float vol_knob_gain_dB = 0.0;      //will be overridden by volume knob
void setup() {
  //begin the serial comms (for debugging)
  myTympan.beginBothSerial();delay(1500);
  Serial.println("Resampling: starting setup()...");
  Serial.print("    : original sample rate (Hz) = ");        Serial.println(original_audio_settings.sample_rate_Hz);
  Serial.print("    : original block size (samples) = ");    Serial.println(original_audio_settings.audio_block_samples);
  Serial.print("    : downsampled sample rate (Hz) = ");     Serial.println(downsampled_audio_settings.sample_rate_Hz);
  Serial.print("    : downsampled block size (samples) = "); Serial.println(downsampled_audio_settings.audio_block_samples);

  // Allocate working memory for audio
  AudioMemory_F32(20, original_audio_settings);

  // setup the downsampling
  const float orig_sample_rate_Hz = original_audio_settings.sample_rate_Hz;
  const float assumed_transition_Hz = 0.05f * orig_sample_rate_Hz;  // assume desired roll-off is always about 5% of the sample rate
  const unsigned int n_downsample_coeff = max(1U, static_cast<unsigned int>(orig_sample_rate_Hz/assumed_transition_Hz));
  downsample.begin(n_downsample_coeff, decimation_factor, original_audio_settings.audio_block_samples);  //give it the input block size

  // setup the upsampling
  const unsigned int upsample_factor = decimation_factor;
  #if 0
    //use better post filtering (more CPU)
    const unsigned int n_upsample_coeff = n_downsample_coeff;
    upsample.begin(n_upsample_coeff, upsample_factor, downsampled_audio_settings.audio_block_samples);  //give it the input block size
  #else
    //use simple linear interpolation (less CPU)
    upsample.begin_linearInterp(upsample_factor, downsampled_audio_settings.audio_block_samples);  //give it the input block size
  #endif

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

      //use the potentiometer as a volume knob
      const float min_val = 0.0, max_val = 40.0; //set desired range
      float new_value = min_val + (max_val - min_val)*val;
      float input_gain_dB = new_value;
      myTympan.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps
      Serial.print("servicePotentiometer: Input Gain (dB) = "); Serial.println(new_value); //print text to Serial port for debugging

    }
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();
