// PitchShifter_FD
//
// Demonstrate pitch shifting via frequency domain processing.
//
// Created: Chip Audette (OpenAudio) Sept 2023
//
// Background: Pitch shifting is when the frequencies within the
//    audio are multiplied up or down such that the harmonic
//    relationships are maintined.  Compare this to "frequency
//    shifting", which also raises or lowers the frequencies of the
//    audio...but frequency shifting simplys adds or subtracts a
//    fixed number of Hz to the audio.  This frequency addition
//    destroys the harmonic relationships within the audio, which
//    makes it unsuitable for musical applications.  Instead, pitch
//    shifting multiplies (not adds) the frequencies, which maintains
//    the harmonic relationships.  Unfortunately, pitch shifting is 
//    much more complicated to implement than frequency shifting.  So,
//    if you look at the code (AudioEffectPitchShift_FD_F32), it is
//    harder to follow.  Listening to the audio, it also has its own
//    unique artifacts...it's not perfect.
//
// Approach: This example uses a phase vocoder implemented in the
//    frequency domain.  Specifically, this uses the two step approach
//    of (1) stretching the audio in time without changing pitch, (2) 
//    resampling the audio [ie, playing it back faster or slower] to
//    return the audio to the original duration, which also ends up
//    raising or lowering the pitch. 
//
//    For raising the pitch, the steps look like this:
//    * Start with samples in the time domain
//    * Stretch the duration while keeping the pitch unchanged: 
//         * Take FFT to convert to frequency domain
//         * Interpolate between FFT blocks to create new FFT blocks 
//         * Take the IFFT and overlap-and-add to create a new (longer) 
//           time-domain signal
//    * Resample the audio to shorten the audio back to the original, 
//      which raises the pitch:
//         * Low-pass filter the data to prevent aliasing that would occur
//           in the next step
//         * Use interpolation to shorten the time-domain data block  back
//           to the original length
//    * Send samples out to the audio interface
//
// Inspired by: https://github.com/JentGent/pitch-shift
//
// Built for the Tympan library for Teensy 4.1-based hardware
//
// Note that this example uses the longest FFT currently allowed with the Tympan 
//     Library (which is 4096, whichis the limit of the underlying ARM DSP library).
//     We need this long of an FFT to provide the frequency resolution necessary for
//     the pitch shifting to sound reasonably good.  Unfortunately, an FFT that is 
//     this long also requires that the length of the audio blocks used by the Tympan
//     be 4096/4 = 1024 points (per channel).  This is "long" and was historically
//     not possible on Tympan.  But, because we wanted to do pitch shifting, we altered
//     the plumbing of the Tympan Library to make it possible.  The solution requires
//     that the user (you) allocate your own data buffers for use by the Tympan Library.
//     This is mildly annoying, but not too bad.  It is all shown in the example below.
//     For additional information regarding the use of "long" audio blocks, see the 
//     Tympan Library example called "LongAudioBlocks". 
//
// MIT License.  Use at your own risk.
//

#include <Tympan_Library.h>
//#include "AudioEffectPitchShift_FD_F32.h"
#include "SerialManager.h"
#include "sample_MUSIC.h"

// set the sample rate and block size
const float sample_rate_Hz = 44100.f;  //choose your desired sample rate
const int audio_block_samples = 1024;  //choose block size.  The FFT size will be 4x the block size.
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

// because we're using block sizes bigger than 128, we need to allocate our own I2S buffers for RX and TX DMA
const int n_chan = 2;  // 2 = stereo, 4 = quad.  Don't choose anything else
uint32_t i2s_rx_buffer[audio_block_samples/2*n_chan]; //allocate the buffer for the InputI2S class
uint32_t i2s_tx_buffer[audio_block_samples/2*n_chan]; //allocate the buffer for the OutputI2S class


// /////////////// Create the audio objects and connect them together

//create audio library objects for handling the audio
Tympan                        audioHardware(TympanRev::E);            //TympanRev::E
AudioInputI2S_F32             i2s_in(audio_settings, i2s_rx_buffer);  //Source of live audio (PCB mic or line-in jack)
AudioPlayMemoryI16_F32        audioPlayMemory(audio_settings);        //pre-recorded example audio included here in sample_MUSIC.h
AudioEffectPitchShift_FD_F32  pitchShift(audio_settings);             //Freq domain processing!  https://github.com/Tympan/Tympan_Library/blob/master/src/AudioEffectpitchShiftFD_F32.h
AudioEffectGain_F32           gain1;                                  //Applies digital gain to audio data.
AudioOutputI2S_F32            i2s_out(audio_settings, i2s_tx_buffer); //Digital audio out *to* the Tympan AIC.

//Make all of the audio connections
#if 0
  //use live audio from the PCB mic or from the pink line-in jack
  AudioConnection_F32       patchCord10(i2s_in, 0, pitchShift, 0);    //use the Left input
#else
  //or, as a demo, use the audio sample included here (will play as a loop)
  AudioConnection_F32       patchCord10(audioPlayMemory, 0, pitchShift, 0); 
#endif
AudioConnection_F32       patchCord20(pitchShift, 0, gain1, 0);   //connect to gain
AudioConnection_F32       patchCord30(gain1, 0, i2s_out, 0);      //connect to the left output
AudioConnection_F32       patchCord40(gain1, 0, i2s_out, 1);      //connect to the right output


// /////////////// Variables and functions to help control the system

//control display and serial interaction
bool enable_printCPUandMemory = false;
void togglePrintMemoryAndCPU(void) { enable_printCPUandMemory = !enable_printCPUandMemory; };
SerialManager serialManager;

//inputs and levels
float input_gain_dB = 15.0f; //gain on the microphone
float vol_knob_gain_dB = 0.0;      //will be overridden by volume knob
void switchToPCBMics(void) {
  Serial.println("Switching to PCB Mics.");
  audioHardware.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the microphone jack - defaults to mic bias OFF
  audioHardware.setInputGain_dB(input_gain_dB);
}
void switchToLineInOnMicJack(void) {
  Serial.println("Switching to Line-in on Mic Jack.");
  audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF  
  audioHardware.setInputGain_dB(0.0);
}
void switchToMicInOnMicJack(void) {
  Serial.println("Switching to Mic-In on Mic Jack.");
  audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack - defaults to mic bias OFF   
  audioHardware.setEnableStereoExtMicBias(true);  //put the mic bias on both channels
  audioHardware.setInputGain_dB(input_gain_dB);
}


// //////////////////////////////// Now the setup() and loop() functions common to all Arduino programs
      
// define the setup() function, the function that is called once when the device is booting
void setup() {
  //audioHardware.beginBothSerial(); 
  delay(500);
  Serial.println("pitchShifter: starting setup()...");
  Serial.print("    : sample rate (Hz) = ");  Serial.println(audio_settings.sample_rate_Hz);
  Serial.print("    : block size (samples) = ");  Serial.println(audio_settings.audio_block_samples);

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory_F32(10, audio_settings);

  // Configure the FFT parameters algorithm
  int overlap_factor = 4;  //always 4 (ie, 75% overlap) for pitch shifting
  int N_FFT = audio_block_samples * overlap_factor;  
  Serial.print("    : N_FFT = "); Serial.println(N_FFT);
  pitchShift.setup(audio_settings, N_FFT); //do after AudioMemory_F32();

  //configure the frequency shifting
  float scale_fac_semitones = +5.0;
  float scale_fac = powf(2.0,scale_fac_semitones/12.0f);
  Serial.println("Setting pitch scale factor to " + String(scale_fac,3));
  pitchShift.setScaleFac(scale_fac); //1.0 is no pitch shifting.
 
  //Enable the Tympan to start the audio flowing!
  audioHardware.enable(); // activate AIC

  //setup DC-blocking highpass filter running in the ADC hardware itself
  float cutoff_Hz = 60.0;  //set the default cutoff frequency for the highpass filter
  audioHardware.setHPFonADC(true,cutoff_Hz,audio_settings.sample_rate_Hz); //set to false to disble

  //Choose the desired input
  switchToPCBMics();        //use PCB mics as input
  //switchToMicInOnMicJack(); //use Mic jack as mic input (ie, with mic bias)
  //switchToLineInOnMicJack();  //use Mic jack as line input (ie, no mic bias)
  
  //Set the desired volume levels
  audioHardware.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
   
  // configure the blue potentiometer
  servicePotentiometer(millis(),0); //update based on the knob setting the "0" is not relevant here.

  //finish the setup by printing the help menu to the serial connections
  serialManager.printHelp();
}


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {
 //start an audio sample?
  if (audioPlayMemory.isPlaying() == false) { //is any audio playing already?
    Serial.println("Starting sample 'MUSIC'...len = " + String(sample_MUSIC_len) + " samples");
    audioPlayMemory.play(sample_MUSIC, sample_MUSIC_len, sample_MUSIC_sample_rate_Hz); //will automatically resample, if needed
  }

  //respond to Serial commands
  while (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial
  //while (Serial1.available()) serialManager.respondToByte((char)Serial1.read()); //BT Serial
  
  //check the potentiometer
  servicePotentiometer(millis(), 100); //service the potentiometer every 100 msec

  //check to see whether to print the CPU and Memory Usage
  if (enable_printCPUandMemory) printCPUandMemory(millis(), 3000); //print every 3000 msec

} //end loop();


// /////////////////////// Here are some servicing routines

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
    float val = float(audioHardware.readPotentiometer()) / 1023.0; //0.0 to 1.0
    val = (1.0/9.0) * (float)((int)(9.0 * val + 0.5)); //quantize so that it doesn't chatter...0 to 1.0

    //use the potentiometer value to control something interesting
    if (abs(val - prev_val) > 0.05) { //is it different than befor?
      prev_val = val;  //save the value for comparison for the next time around

      //change the volume
      float vol_dB = 0.f + 30.0f * ((val - 0.5) * 2.0); //set volume as 0dB +/- 30 dB
      audioHardware.print("Changing output volume to = "); audioHardware.print(vol_dB); audioHardware.println(" dB");
      audioHardware.volume_dB(vol_dB);
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
    Serial.print("printCPUandMemory: ");
    Serial.print("CPU Cur/Peak: ");
    Serial.print(audio_settings.processorUsage());
    Serial.print("%/");
    Serial.print(audio_settings.processorUsageMax());
    Serial.print("%,   ");
    Serial.print("Dyn MEM Float32 Cur/Peak: ");
    Serial.print(AudioMemoryUsage_F32());
    Serial.print("/");
    Serial.print(AudioMemoryUsageMax_F32());
    Serial.println();

    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}

void printGainSettings(void) {
  Serial.print("Gain (dB): ");
  Serial.print("Vol Knob = "); Serial.print(vol_knob_gain_dB,1);
  //Serial.print(", Input PGA = "); Serial.print(input_gain_dB,1);
  Serial.println();
}

// /////////////////////////////// Here are some fuctions that help control the system (such as from the Serial Monitor)

void incrementKnobGain(float increment_dB) { //"extern" to make it available to other files, such as SerialManager.h
  setVolKnobGain_dB(vol_knob_gain_dB+increment_dB);
}

void setVolKnobGain_dB(float gain_dB) {
  vol_knob_gain_dB = gain_dB;
  gain1.setGain_dB(vol_knob_gain_dB);
  printGainSettings();
}

float incrementPitchShift(float32_t incr_factor) {
  float scale_fac = pitchShift.getScaleFac();
  return pitchShift.setScaleFac(scale_fac * incr_factor);
}
