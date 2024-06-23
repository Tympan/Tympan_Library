// FreqShifter_FD
//
// Demonstrate frequency shifting via frequency domain processing.
//
// Created: Chip Audette (OpenAudio) Aug 2019
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
// Frequency Domain Processing:
//    * Take samples in the time domain
//    * Take FFT to convert to frequency domain
//    * Manipulate the frequency bins to do the freqyebct shifting
//    * Take IFFT to convert back to time domain
//    * Send samples back to the audio interface
//
// Built for the Tympan library for Teensy 3.6-based hardware
//
// MIT License.  Use at your own risk.
//

#include <Tympan_Library.h>
#include "SerialManager.h"

//set the sample rate and block size
const float sample_rate_Hz = 44100.f; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 64;     //for freq domain processing choose a power of 2 (16, 32, 64, 128) but no higher than 128
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
Tympan                        myTympan(TympanRev::F, audio_settings);   //do TympanRev::D or E or F
AudioInputI2S_F32             i2s_in(audio_settings);          //Digital audio *from* the Tympan AIC.
AudioEffectFreqShift_FD_F32   freqShift(audio_settings);       //Freq domain processing!  https://github.com/Tympan/Tympan_Library/blob/master/src/AudioEffectFreqShiftFD_F32.h
AudioEffectGain_F32           gain1;                           //Applies digital gain to audio data.
AudioOutputI2S_F32            i2s_out(audio_settings);         //Digital audio out *to* the Tympan AIC.

//Make all of the audio connections
AudioConnection_F32       patchCord10(i2s_in, 0, freqShift, 0);   //use the Left input
AudioConnection_F32       patchCord20(freqShift, 0, gain1, 0);     //connect to gain
AudioConnection_F32       patchCord30(gain1, 0, i2s_out, 0);      //connect to the left output
AudioConnection_F32       patchCord40(gain1, 0, i2s_out, 1);       //connect to the right output


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
  mySerial.println("freqShifter: starting setup()...");
  mySerial.print("    : sample rate (Hz) = ");  mySerial.println(audio_settings.sample_rate_Hz);
  mySerial.print("    : block size (samples) = ");  mySerial.println(audio_settings.audio_block_samples);

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory_F32(40, audio_settings);

  // Configure the FFT parameters algorithm
  int overlap_factor = 4;  //set to 2, 4 or 8...which yields 50%, 75%, or 87.5% overlap (8x)
  int N_FFT = audio_block_samples * overlap_factor;  
  Serial.print("    : N_FFT = "); Serial.println(N_FFT);
  freqShift.setup(audio_settings, N_FFT); //do after AudioMemory_F32();

  //configure the frequency shifting
  float shiftFreq_Hz = 750.0; //shift audio upward a bit
  float Hz_per_bin = audio_settings.sample_rate_Hz / ((float)N_FFT);
  int shift_bins = (int)(shiftFreq_Hz / Hz_per_bin + 0.5);  //round to nearest bin
  shiftFreq_Hz = shift_bins * Hz_per_bin;
  Serial.print("Setting shift to "); Serial.print(shiftFreq_Hz); Serial.print(" Hz, which is "); Serial.print(shift_bins); Serial.println(" bins");
  freqShift.setShift_bins(shift_bins); //0 is no ffreq shifting.
 
  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate AIC

  //setup DC-blocking highpass filter running in the ADC hardware itself
  float cutoff_Hz = 60.0;  //set the default cutoff frequency for the highpass filter
  myTympan.setHPFonADC(true,cutoff_Hz,audio_settings.sample_rate_Hz); //set to false to disble

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
    mySerial.print(audio_settings.processorUsage());
    mySerial.print("%/");
    mySerial.print(audio_settings.processorUsageMax());
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
  int cur_shift_bins = freqShift.getShift_bins();
  return freqShift.setShift_bins(cur_shift_bins + incr_factor);
}
