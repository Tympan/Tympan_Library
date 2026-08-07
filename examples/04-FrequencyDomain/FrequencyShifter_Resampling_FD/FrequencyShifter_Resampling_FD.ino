// FreqShifter_Resampling_FD
//
// Demonstrate frequency shifting via frequency domain processing.
//  Also use the frequency-shifter to decimate the signal.
//  Perform additional time-domain processing on the decimatd signal.
//  Upsample to get back to original sample rate to send to output
//
// Created: Chip Audette (OpenAudio) Aug 2026
//
// Approach: This processing is performed in the frequency domain.
//    Frequencies can only be shifted by an integer number of bins,
//    so small frequency shifts are not possible.  For example, for
//    a sample rate of 44.1kHz, and when using N=256, one can only
//    shift frequencies in multiples of 44.1/256 = 172.3 Hz.
//
//    This processing is performed in the frequency domain where
//    we take the FFT, shift the bins upward or downward, take
//    the IFFT, and listen to the results.  In effect, this is
//    single sideband modulation, which will sound very unnatural
//    (like robot voices!).  Maybe you'll like it, or maybe not.
//    Probably not, unless you like weird.  ;)
//
//    You can shift frequencies upward or downward with this algorithm.
//
// CPU Usage: original sample rate = 96 kHz running WDRC comp and gain
//   USE_DECIMATION = false, decimation_factor = 1.  CPU = 16.2%
//   USE_DECIMATION = true,  decimation_factor = 4.  CPU = 9.3% [6.9% savings]
//    
// The decimation is performed by the frequency shifter where it performs
//    an IFFT that is shorter than the FFT that it started with.  It discards
//    all the higher bins when it does the shorter IFFT.
//
// MIT License.  Use at your own risk.
//

#include <Tympan_Library.h>
#include "SerialManager.h"

#define USE_DECIMATION true

// //////////////////////////////////////////////////// IMPORTANT: SETTING THE SAMPLE RATES
// Here's where we choose our different sampling rates.

//set the sample rate and block size to/from the hardware
constexpr float original_sample_rate_Hz = 96000.f;
constexpr int original_audio_block_samples = 128;     //for freq domain processing choose a power of 2 (16, 32, 64, 128) but no higher than 128
AudioSettings_F32 original_audio_settings(original_sample_rate_Hz, original_audio_block_samples);

// set the reduced sample rate to use for intermediate calculationsc
#if USE_DECIMATION
  constexpr unsigned int decimation_factor = 4;    // By how much do we want to decimate the sample rate?  As the decimation is in the frequency domain, choose a power of 2.
#else
  constexpr unsigned int decimation_factor = 1;    // By how much do we want to decimate the sample rate?  As the decimation is in the frequency domain, choose a power of 2.
#endif
constexpr float downsampled_sample_rate_Hz = original_sample_rate_Hz / decimation_factor;
constexpr int downsampled_audio_block_samples = original_audio_block_samples / decimation_factor;     //for freq domain processing choose a power of 2 (16, 32, 64, 128) but no higher than 128
AudioSettings_F32 downsampled_audio_settings(downsampled_sample_rate_Hz, downsampled_audio_block_samples);

// ////////////////////////////////////////////// CONTINUING WITH NORMAL SETUP...

//create audio library objects for handling the audio
Tympan                        myTympan(TympanRev::F, original_audio_settings);   //do TympanRev::D or E or F

//create audio classes paying attention to whether they start with original or decimated sample rate
AudioInputI2S_F32             i2s_in(          original_audio_settings);     //Digital audio *from* the Tympan AIC.
AudioEffectFreqShift_FD_F32   shiftAndDecimate(original_audio_settings);     //Freq domain processing!  https://github.com/Tympan/Tympan_Library/blob/master/src/AudioEffectFreqShiftFD_F32.h
AudioEffectCompWDRC_F32       comp1(           downsampled_audio_settings);  //processing that uses a good amount of CPU
AudioEffectGain_F32           gain1(           downsampled_audio_settings);  //simple digital gain
AudioRateInterpolator_F32     upsample(        downsampled_audio_settings);  //use the audio_settings for the audio coming *into* the object (but doesn't really matter as we'll re-initialize via setup() later)  
AudioOutputI2S_F32            i2s_out(         original_audio_settings);     //Digital audio *to* the Tympan AIC hardware.

//Make all of the audio connections
#if USE_DECIMATION
  //use decimation
  AudioConnection_F32       patchCord1(i2s_in, 0, shiftAndDecimate, 0);  //left into to shifter/decimator
  AudioConnection_F32       patchCord2(shiftAndDecimate, 0, comp1, 0);   //connect to compressor
  AudioConnection_F32       patchCord3(comp1, 0, gain1, 0);              //connect to gain
  AudioConnection_F32       patchCord4(gain1, 0, upsample, 0);           //connect to upsampler
  AudioConnection_F32       patchCord5(upsample, 0, i2s_out, 0);         //connect to the left output
  AudioConnection_F32       patchCord6(upsample, 0, i2s_out, 1);         //connect to the right output
#else
  //don't use the decimation
  AudioConnection_F32       patchCord1(i2s_in, 0, shiftAndDecimate, 0);   //left into to shifter/decimator
  AudioConnection_F32       patchCord2(shiftAndDecimate, 0, comp1, 0);    //connect to compressor
  AudioConnection_F32       patchCord3(comp1, 0, gain1, 0);               //connect to gain
  AudioConnection_F32       patchCord4(gain1, 0, i2s_out, 0);             //connect to the left output
  AudioConnection_F32       patchCord5(gain1, 0, i2s_out, 1);             //connect to the right output
#endif

//control display and serial interaction
bool enable_printCPUandMemory = false;
void togglePrintMemoryAndCPU(void) { enable_printCPUandMemory = !enable_printCPUandMemory; };
SerialManager serialManager(myTympan);
#define mySerial myTympan   //myTympan is a printable stream!

//inputs and levels
float input_gain_dB = 15.0f; //gain on the microphone
float vol_knob_gain_dB = 0.0;      //will be overridden by volume knob
void switchToPCBMics(void) {
  mySerial.println("Switching to PCB Mics.");
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the microphone jack - defaults to mic bias OFF
  myTympan.setInputGain_dB(input_gain_dB);
}
void switchToLineInOnMicJack(void) {
  mySerial.println("Switching to Line-in on Mic Jack.");
  myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF  
  myTympan.setInputGain_dB(0.0);
}
void switchToMicInOnMicJack(void) {
  mySerial.println("Switching to Mic-In on Mic Jack.");
  myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack - defaults to mic bias OFF   
  myTympan.setEnableStereoExtMicBias(true);  //put the mic bias on both channels
  myTympan.setInputGain_dB(input_gain_dB);
}
      
// define the setup() function, the function that is called once when the device is booting
void setup() {
  myTympan.beginBothSerial(); delay(1000);
  Serial.println("Resampling: starting setup()...");
  Serial.print("    : original sample rate (Hz) = ");        Serial.println(original_audio_settings.sample_rate_Hz);
  Serial.print("    : original block size (samples) = ");    Serial.println(original_audio_settings.audio_block_samples);
  Serial.print("    : downsampled sample rate (Hz) = ");     Serial.println(downsampled_audio_settings.sample_rate_Hz);
  Serial.print("    : downsampled block size (samples) = "); Serial.println(downsampled_audio_settings.audio_block_samples);

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory_F32(40, original_audio_settings);

  // Configure the FFT parameters algorithm
  int overlap_factor = 4;  //set to 2, 4 or 8...which yields 50%, 75%, or 87.5% overlap (8x)
  const int N_FFT = original_audio_block_samples * overlap_factor;  
  const int N_IFFT = N_FFT / decimation_factor;  //if you want to decimate, specify your N_IFFT by dividing N_FFT by your decimation factor
  Serial.println("    : N_FFT = " + String(N_FFT));
  Serial.println("    : N_IFFT = " + String(N_IFFT));
  shiftAndDecimate.setup(original_audio_settings, N_FFT, N_IFFT); //do after AudioMemory_F32();

  //configure the frequency shifting
  const float nyquist_Hz =  original_sample_rate_Hz/2.0f;
  float shiftFreq_Hz = -( nyquist_Hz * ( 1.0f - (1.0f/decimation_factor) ) ); //get the highest-frequency range of audio and pull it down to fit within our decimated audio bandwidth
  shiftFreq_Hz = min(shiftFreq_Hz, -750.0);  //ensure that we're always shifting a bit...shifting by 0 might mess up the CPU calcs
  const float Hz_per_bin = original_sample_rate_Hz / ((float)N_FFT);
  int shift_bins = (int)(shiftFreq_Hz / Hz_per_bin + 0.5);  //round to nearest bin
  shiftFreq_Hz = shift_bins * Hz_per_bin;
  Serial.println("Setting shift to " + String(shiftFreq_Hz) + " Hz, which is " + String(shift_bins) + " bins");
  shiftAndDecimate.setShift_bins(shift_bins); //0 is no ffreq shifting.
 
  // setup the upsampling
  #if USE_DECIMATION
    const unsigned int upsample_factor = decimation_factor;
    upsample.begin_linearInterp(upsample_factor, downsampled_audio_settings.audio_block_samples);  //give it the input block size
  #endif

  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate AIC

  //setup DC-blocking highpass filter running in the ADC hardware itself
  float cutoff_Hz = 60.0;  //set the default cutoff frequency for the highpass filter
  myTympan.setHPFonADC(true,cutoff_Hz,original_audio_settings.sample_rate_Hz); //set to false to disble

  //Choose the desired input
  switchToPCBMics();        //use PCB mics as input
  //switchToMicInOnMicJack(); //use Mic jack as mic input (ie, with mic bias)
  //switchToLineInOnMicJack();  //use Mic jack as line input (ie, no mic bias)
  
  //Set the desired volume levels
  myTympan.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
   
  // configure the blue potentiometer
  servicePotentiometer(millis(),0); //update based on the knob setting the "0" is not relevant here.

  //finish the setup by printing the help menu to the serial connections
  serialManager.printHelp();
}


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //respond to Serial commands
  while (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial
  //while (Serial1.available()) serialManager.respondToByte((char)Serial1.read()); //BT Serial
  
  //check the potentiometer
  servicePotentiometer(millis(), 100); //service the potentiometer every 100 msec

  //check to see whether to print the CPU and Memory Usage
  if (enable_printCPUandMemory) printCPUandMemory(millis(), 3000); //print every 3000 msec

} //end loop();


// ///////////////// Servicing routines

//servicePotentiometer: listens to the blue potentiometer and sends the new pot value
//  to the audio processing algorithm as a control parameter
void servicePotentiometer(unsigned long curTime_millis, const unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 100; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;
  static float prev_val = -1.0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    //read potentiometer
    float val = float(myTympan.readPotentiometer()) / 1023.0; //0.0 to 1.0
    val = (1.0/9.0) * (float)((int)(9.0 * val + 0.5)); //quantize so that it doesn't chatter...0 to 1.0

    //use the potentiometer value to control something interesting
    if (abs(val - prev_val) > 0.05) { //is it different than befor?
      prev_val = val;  //save the value for comparison for the next time around

      //change the volume
      float vol_dB = 0.f + 30.0f * ((val - 0.5) * 2.0); //set volume as 0dB +/- 30 dB
      myTympan.print("Changing output volume to = "); myTympan.print(vol_dB); myTympan.println(" dB");
      myTympan.volume_dB(vol_dB);

    }

    
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();



//This routine prints the current and maximum CPU usage and the current usage of the AudioMemory that has been allocated
void printCPUandMemory(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 3000; //how many milliseconds between updating gain reading?
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    mySerial.print("printCPUandMemory: ");
    mySerial.print("CPU Cur/Peak: ");
    mySerial.print(original_audio_settings.processorUsage());
    mySerial.print("%/");
    mySerial.print(original_audio_settings.processorUsageMax());
    mySerial.print("%,   ");
    mySerial.print("Dyn MEM Float32 Cur/Peak: ");
    mySerial.print(AudioMemoryUsage_F32());
    mySerial.print("/");
    mySerial.print(AudioMemoryUsageMax_F32());
    mySerial.println();

    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}

void printGainSettings(void) {
  mySerial.print("Gain (dB): ");
  mySerial.print("Vol Knob = "); mySerial.print(vol_knob_gain_dB,1);
  //mySerial.print(", Input PGA = "); mySerial.print(input_gain_dB,1);
  mySerial.println();
}


void incrementKnobGain(float increment_dB) { //"extern" to make it available to other files, such as SerialManager.h
  setVolKnobGain_dB(vol_knob_gain_dB+increment_dB);
}

void setVolKnobGain_dB(float gain_dB) {
  vol_knob_gain_dB = gain_dB;
  gain1.setGain_dB(vol_knob_gain_dB);
  printGainSettings();
}

int incrementFreqShift(int incr_factor) {
  int cur_shift_bins = shiftAndDecimate.getShift_bins();
  return shiftAndDecimate.setShift_bins(cur_shift_bins + incr_factor);
}
