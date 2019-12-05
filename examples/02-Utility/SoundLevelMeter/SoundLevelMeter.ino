/*
*   SoundLevelMeter
*
*   Created: Chip Audette, OpenAudio, June 2018
*   Purpose: Compute the current sound level, dBA-Fast or whatever
*            Uses exponential time weighting, not integrating.
*
*   Uses Tympan Audio Adapter.
*
*   MIT License.  use at your own risk.
*/

//here are the libraries that we need
#include <Tympan_Library.h>  //include the Tympan Library
#include "SerialManager.h"

//set the sample rate and block size
const float sample_rate_Hz = 44117.0f ; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 128;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
Tympan                          myTympan(TympanRev::D);   //use TympanRev::D or TympanRev::C
AudioInputI2S_F32               i2s_in(audio_settings);       //Digital audio in *from* the Teensy Audio Board ADC.
AudioFilterFreqWeighting_F32    freqWeight1(audio_settings);  //A-weighting filter (optionally C-weighting)
AudioCalcLevel_F32              calcLevel1(audio_settings);    //use this to square the signal
AudioRecordQueue_F32            audioQueue1(audio_settings);  //use this to get access to the data
AudioOutputI2S_F32              i2s_out(audio_settings);      //Digital audio out *to* the Teensy Audio Board DAC.

//Make all of the audio connections
AudioConnection_F32       patchCord1(i2s_in, 0, freqWeight1, 0);   //connect the Left input
AudioConnection_F32       patchCord2(freqWeight1, 0, calcLevel1, 0); //rectify and time weighting  
AudioConnection_F32       patchCord3(calcLevel1, 0, audioQueue1, 0); //to allow for printing
AudioConnection_F32       patchCord4(i2s_in, 0, i2s_out, 0);      //echo the original signal
AudioConnection_F32       patchCord5(calcLevel1, 0, i2s_out, 1);     //connect the Right gain to the Right output

//calibraiton information for the microphone being used
//float32_t mic_cal_dBFS_at94dBSPL_at_0dB_gain = -47.4f ;  //PCB Mic, http://openaudio.blogspot.com/search/label/Microphone
float32_t mic_cal_dBFS_at94dBSPL_at_0dB_gain = -47.4f + 9.2175;  //PCB Mic baseline with manually tested adjustment.   Baseline:  http://openaudio.blogspot.com/search/label/Microphone

//other variables
float32_t cur_audio_pow = -99.9f;   //initialize to any number less than zero
float32_t max_audio_pow = 0.0f;     //initilize to zero (or some small number)  

//control display and serial interaction
bool enable_printCPUandMemory = false;
void enablePrintMemoryAndCPU(bool _enable) { enable_printCPUandMemory = _enable; }
bool enable_printLoudnessLevels = true; 
void enablePrintLoudnessLevels(bool _enable) { 
  enable_printLoudnessLevels = _enable; 
  max_audio_pow = cur_audio_pow;  //reset the max loudness state, too
};
SerialManager serialManager;



// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 15.0f; //gain on the microphone
void setup() {
  //begin the serial comms (for debugging)
  //Serial.begin(115200);  delay(500);
  myTympan.beginBothSerial(); delay(500);
  myTympan.println("SoundLevelMeter: Starting setup()...");

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(20,audio_settings); 

  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate AIC
 
  //Choose the desired input
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  myTympan.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  myTympan.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  //define the sound level meter processing
  if (1) {
    freqWeight1.setWeightingType(A_WEIGHT);        //A_WEIGHT or C_WEIGHT
    myTympan.println("Frequency Weighting: A_WEIGHT");
  } else {
    freqWeight1.setWeightingType(C_WEIGHT);        //A_WEIGHT or C_WEIGHT
    myTympan.println("Frequency Weighting: C_WEIGHT");
  }
  if (1) {
    calcLevel1.setTimeConst_sec(TIME_CONST_SLOW); //TIME_CONST_SLOW or TIME_CONST_FAST or use a value (seconds)
    myTympan.println("Time Weighting: SLOW");
  } else {
    calcLevel1.setTimeConst_sec(TIME_CONST_FAST); //TIME_CONST_SLOW or TIME_CONST_FAST or use a value (seconds)
    myTympan.println("Time Weighting: FAST");
  }

  //enable any of the other algorithm elements
  audioQueue1.begin();


  // check the volume knob
  //servicePotentiometer(millis(),0);  //the "0" is not relevant here.

  myTympan.println("Setup complete.");
  serialManager.printHelp();
  
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {
  //choose to sleep ("wait for interrupt") instead of spinning our wheels doing nothing but consuming power
  //asm(" WFI");  //ARM-specific.  Will wake on next interrupt.  The audio library issues tons of interrupts, so we wake up often.

  //service record queue
  serviceAudioQueue();

  //respond to Serial commands
  while (Serial.available()) serialManager.respondToByte((char)Serial.read());
  while (Serial1.available()) serialManager.respondToByte((char)Serial1.read());
  
  //printing of sound level
  if (enable_printLoudnessLevels) printLoudnessLevels(millis(),250);  //print a value every 250 msec

  //check to see whether to print the CPU and Memory Usage
  if (enable_printCPUandMemory) printCPUandMemory(millis(),3000); //print every 3000 msec

} //end loop();

// ///////////////// Servicing routines

//serviceAudioQueue: whenever a queue is available, get the maximum value and the latest value
void serviceAudioQueue(void) {
  float32_t *buff;
  while (audioQueue1.available()) {
    buff = audioQueue1.readBuffer();
    for (int i=0; i < audio_block_samples; i++) {
      if (buff[i] > max_audio_pow) max_audio_pow = buff[i];
    }
    cur_audio_pow = buff[audio_block_samples-1];
    audioQueue1.freeBuffer();
  }
}

//This routine prints the current SPL and the maximum since the last printing
void printLoudnessLevels(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static bool firstTime = true;
  static unsigned long lastUpdate_millis = 0;
  if (cur_audio_pow < 0.0f) return;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    if (firstTime) {
      myTympan.println("Printing: current SPL (dB), max SPL (dB)");
      firstTime = false;
    }
    float32_t cal_factor_dB = -mic_cal_dBFS_at94dBSPL_at_0dB_gain + 94.0f - input_gain_dB;
    float32_t cur_SPL_dB = 10.0f*log10f(cur_audio_pow) + cal_factor_dB;
    float32_t max_SPL_dB = 10.0f*log10f(max_audio_pow) + cal_factor_dB;
    Serial1.print("E "); //for plotting by bluetooth app
    myTympan.print(cur_SPL_dB);
    myTympan.print(", ");
    myTympan.print(max_SPL_dB);
    myTympan.println();
    
    max_audio_pow = 0.0;  //recent for next block
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}


//This routine prints the current and maximum CPU usage and the current usage of the AudioMemory that has been allocated
void printCPUandMemory(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    myTympan.print("printCPUandMemory: ");
    myTympan.print("CPU Cur/Peak: ");
    myTympan.print(audio_settings.processorUsage());
    myTympan.print("%/");
    myTympan.print(audio_settings.processorUsageMax());
    myTympan.print("%,   ");
    myTympan.print("Dyn MEM Float32 Cur/Peak: ");
    myTympan.print(AudioMemoryUsage_F32());
    myTympan.print("/");
    myTympan.print(AudioMemoryUsageMax_F32());
    myTympan.println();

    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}
