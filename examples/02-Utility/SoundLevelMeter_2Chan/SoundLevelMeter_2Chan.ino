/*
*   SoundLevelMeter_2Chan
*
*   Created: Chip Audette, OpenAudio 
*   Original: SoundLevelMeter (1 channel), June 2018
*   Updated: Expanded to two channel, running at 96 kHz, March 2019
*   
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
const float sample_rate_Hz = 96000.0f ; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 128;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
Tympan                          audioHardware(TympanRev::D);   //use TympanRev::D or TympanRev::C
AudioInputI2S_F32               i2s_in(audio_settings);       //Digital audio in *from* the Teensy Audio Board ADC.
AudioFilterFreqWeighting_F32    freqWeight1(audio_settings),freqWeight2(audio_settings);  //A-weighting filter (optionally C-weighting)
AudioCalcLevel_F32              calcLevel1(audio_settings),calcLevel2(audio_settings);    //use this to square the signal
AudioRecordQueue_F32            audioQueue1(audio_settings),audioQueue2(audio_settings);  //use this to get access to the data
AudioOutputI2S_F32              i2s_out(audio_settings);      //Digital audio out *to* the Teensy Audio Board DAC.

//Make all of the audio connections
AudioConnection_F32       patchCord1(i2s_in, 0, freqWeight1, 0);   //connect the Left input
AudioConnection_F32       patchCord2(freqWeight1, 0, calcLevel1, 0); //rectify and time weighting  
AudioConnection_F32       patchCord3(calcLevel1, 0, audioQueue1, 0); //to allow for printing

AudioConnection_F32       patchCord11(i2s_in, 1, freqWeight2, 0);   //connect the Right input
AudioConnection_F32       patchCord12(freqWeight2, 0, calcLevel2, 0); //rectify and time weighting  
AudioConnection_F32       patchCord13(calcLevel2, 0, audioQueue2, 0); //to allow for printing

//choose what to send out the audio outputs
#if 1
AudioConnection_F32       patchCord4(i2s_in, 0, i2s_out, 0);      //echo the LEFT input to the LEFT output
AudioConnection_F32       patchCord5(i2s_in, 1, i2s_out, 1);     //echo the RIGHT input to the RIGHT output
#else
AudioConnection_F32       patchCord4(i2s_in, 0, i2s_out, 0);      //echo the original LEFT signal
AudioConnection_F32       patchCord5(calcLevel1, 0, i2s_out, 1);     //connect the LEFT level to the Right audio output
#endif

//calibraiton information for the microphone being used...but this should really be a per-frequency weighting!!!!
//float32_t mic_cal_dBFS_at94dBSPL_at_0dB_gain = -47.4f ;  //PCB Mic, http://openaudio.blogspot.com/search/label/Microphone
float32_t mic1_cal_dBFS_at94dBSPL_at_0dB_gain = -47.4f + 9.2175;  //PCB Mic baseline with manually tested adjustment.   Baseline:  http://openaudio.blogspot.com/search/label/Microphone
float32_t mic2_cal_dBFS_at94dBSPL_at_0dB_gain = -47.4f + 9.2175;  //PCB Mic baseline with manually tested adjustment.   Baseline:  http://openaudio.blogspot.com/search/label/Microphone

//other variables
#define BOTH_SERIAL audioHardware
float32_t cur_audio_pow[2] = {-99.9f, -99.9f};   //initialize to any number less than zero
float32_t max_audio_pow[2] = {0.0f, 0.0f};     //initilize to zero (or some small number)  

//control display and serial interaction
bool enable_printCPUandMemory = false;
void enablePrintMemoryAndCPU(bool _enable) { enable_printCPUandMemory = _enable; }
bool enable_printLoudnessLevels = true; 
void enablePrintLoudnessLevels(bool _enable) { 
  enable_printLoudnessLevels = _enable; 
  max_audio_pow[0] = cur_audio_pow[0];  //reset the max loudness state, too
  max_audio_pow[1] = cur_audio_pow[1];  //reset the max loudness state, too
};
SerialManager serialManager(audioHardware);


// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 15.0f; //gain on the microphone
void setup() {
  //begin the serial comms (for debugging)
  //Serial.begin(115200);  delay(500);
  audioHardware.beginBothSerial(); delay(500);
  BOTH_SERIAL.println("SoundLevelMeter: Starting setup()...");

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(40,audio_settings); 

  //Enable the Tympan to start the audio flowing!
  audioHardware.enable(); // activate AIC
 
  //Choose the desired input
  audioHardware.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
  //audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
  //audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  audioHardware.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  audioHardware.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  //define the sound level meter processing
  if (1) {
    freqWeight1.setWeightingType(A_WEIGHT);        //A_WEIGHT or C_WEIGHT
    freqWeight2.setWeightingType(A_WEIGHT);        //A_WEIGHT or C_WEIGHT
    BOTH_SERIAL.println("Frequency Weighting: A_WEIGHT");
  } else {
    freqWeight1.setWeightingType(C_WEIGHT);        //A_WEIGHT or C_WEIGHT
    freqWeight2.setWeightingType(C_WEIGHT);        //A_WEIGHT or C_WEIGHT
    BOTH_SERIAL.println("Frequency Weighting: C_WEIGHT");
  }
  if (1) {
    calcLevel1.setTimeConst_sec(TIME_CONST_SLOW); //TIME_CONST_SLOW or TIME_CONST_FAST or use a value (seconds)
    calcLevel2.setTimeConst_sec(TIME_CONST_SLOW); //TIME_CONST_SLOW or TIME_CONST_FAST or use a value (seconds)
    BOTH_SERIAL.println("Time Weighting: SLOW");
  } else {
    calcLevel1.setTimeConst_sec(TIME_CONST_FAST); //TIME_CONST_SLOW or TIME_CONST_FAST or use a value (seconds)
    calcLevel2.setTimeConst_sec(TIME_CONST_FAST); //TIME_CONST_SLOW or TIME_CONST_FAST or use a value (seconds)
    BOTH_SERIAL.println("Time Weighting: FAST");
  }

  //enable any of the other algorithm elements
  audioQueue1.begin();
  audioQueue2.begin();

  BOTH_SERIAL.println("Setup complete.");
  serialManager.printHelp();
  
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {
  //choose to sleep ("wait for interrupt") instead of spinning our wheels doing nothing but consuming power
  asm(" WFI");  //ARM-specific.  Will wake on next interrupt.  The audio library issues tons of interrupts, so we wake up often.

  //service record queue
  serviceAudioQueue();

  //respond to Serial commands
  while (Serial.available()) serialManager.respondToByte((char)Serial.read());
  while (Serial1.available()) serialManager.respondToByte((char)Serial1.read());
  
  //printing of sound level
  if (enable_printLoudnessLevels) printLoudnessLevels(millis(),250);  //print a value every 250 msec

  //check the potentiometer
  //servicePotentiometer(millis(),100); //service the potentiometer every 100 msec

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
      if (buff[i] > max_audio_pow[0]) max_audio_pow[0] = buff[i];
    }
    cur_audio_pow[0] = buff[audio_block_samples-1];
    audioQueue1.freeBuffer();
  }
  while (audioQueue2.available()) {
    buff = audioQueue2.readBuffer();
    for (int i=0; i < audio_block_samples; i++) {
      if (buff[i] > max_audio_pow[1]) max_audio_pow[1] = buff[i];
    }
    cur_audio_pow[1] = buff[audio_block_samples-1];
    audioQueue2.freeBuffer();
  }
}

//This routine prints the current SPL and the maximum since the last printing
void printLoudnessLevels(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static bool firstTime = true;
  static unsigned long lastUpdate_millis = 0;
  if (cur_audio_pow[0] < 0.0f) return;  //should this be modified to check both audio channels?

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    if (firstTime) {
      BOTH_SERIAL.println("Printing: current Left SPL (dB), max Left SPL (dB), current Right SPL (dB), max Right SPL (dB), ");
      firstTime = false;
    }
    Serial1.print("E "); //send only out the bluetooth link (for plotting by bluetooth app)

    //Channel 1
    float32_t cal_factor1_dB = -mic1_cal_dBFS_at94dBSPL_at_0dB_gain + 94.0f - input_gain_dB;
    float32_t cur_SPL1_dB = 10.0f*log10f(cur_audio_pow[0]) + cal_factor1_dB;
    float32_t max_SPL1_dB = 10.0f*log10f(max_audio_pow[0]) + cal_factor1_dB;
    BOTH_SERIAL.print(cur_SPL1_dB);
    BOTH_SERIAL.print(", ");
    BOTH_SERIAL.print(max_SPL1_dB);

    //channel 2
    float32_t cal_factor2_dB = -mic2_cal_dBFS_at94dBSPL_at_0dB_gain + 94.0f - input_gain_dB;
    float32_t cur_SPL2_dB = 10.0f*log10f(cur_audio_pow[1]) + cal_factor2_dB;
    float32_t max_SPL2_dB = 10.0f*log10f(max_audio_pow[1]) + cal_factor2_dB;  
    BOTH_SERIAL.print(", ");    
    BOTH_SERIAL.print(cur_SPL2_dB);
    BOTH_SERIAL.print(", ");
    BOTH_SERIAL.print(max_SPL2_dB);
    
    //finish up
    BOTH_SERIAL.println();
    
    max_audio_pow[0] = 0.0;  //recent for next block
    max_audio_pow[1] = 0.0;  //recent for next block
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}

//This routine prints the current and maximum CPU usage and the current usage of the AudioMemory that has been allocated
void printCPUandMemory(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    BOTH_SERIAL.print("printCPUandMemory: ");
    BOTH_SERIAL.print("CPU Cur/Peak: ");
    BOTH_SERIAL.print(audio_settings.processorUsage());
    //BOTH_SERIAL.print(AudioProcessorUsage()); //if not using AudioSettings_F32
    BOTH_SERIAL.print("%/");
    BOTH_SERIAL.print(audio_settings.processorUsageMax());
    //BOTH_SERIAL.print(AudioProcessorUsageMax());  //if not using AudioSettings_F32
    BOTH_SERIAL.print("%,   ");
    BOTH_SERIAL.print("Dyn MEM Float32 Cur/Peak: ");
    BOTH_SERIAL.print(AudioMemoryUsage_F32());
    BOTH_SERIAL.print("/");
    BOTH_SERIAL.print(AudioMemoryUsageMax_F32());
    BOTH_SERIAL.println();

    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}

