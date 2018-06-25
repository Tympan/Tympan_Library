/*
*   TrebleBoost
*
*   Created: Chip Audette, OpenAudio, Apr 2017
*   Purpose: Process audio by applying a high-pass filter followed by gain
*
*   Uses Tympan Audio Adapter.
*   Blue potentiometer adjusts the digital gain applied to the filtered audio signal.
*
*   MIT License.  use at your own risk.
*/

//here are the libraries that we need
#include <Tympan_Library.h>  //include the Tympan Library

//set the sample rate and block size
const float sample_rate_Hz = 44117.0f ; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 128;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
TympanPins                      tympPins(TYMPAN_REV_C);        //TYMPAN_REV_C or TYMPAN_REV_D
TympanBase                      audioHardware(tympPins);
AudioInputI2S_F32               i2s_in(audio_settings);       //Digital audio in *from* the Teensy Audio Board ADC.
AudioFilterFreqWeighting_F32    freqWeight1(audio_settings);  //A-weighting filter (optionally C-weighting)
AudioCalcLevel_F32              calcLevel1(audio_settings);    //use this to square the signal
AudioRecordQueue_F32            audioQueue1(audio_settings);  //use this to get access to the data
AudioOutputI2S_F32              i2s_out(audio_settings);      //Digital audio out *to* the Teensy Audio Board DAC.

//Make all of the audio connections
AudioConnection_F32       patchCord1(i2s_in, 0, freqWeight1, 0);   //connect the Left input
AudioConnection_F32       patchCord2(freqWeight1, 0, calcLevel1, 0); //rectify and time weighting  
AudioConnection_F32       patchCord5(i2s_in, 0, i2s_out, 0);      //echo the original signal
AudioConnection_F32       patchCord6(calcLevel1, 0, i2s_out, 1);     //connect the Right gain to the Right output

//other variables
#define BOTH_SERIAL  audioHardware  //TympanBase supports print()/println(), so this simplifies printing to both USB and BT Serial
float32_t cur_audio_pow = -10.0f, max_audio_pow = 0.0f;

//I have a potentiometer on the Teensy Audio Board
#define POT_PIN A1  //potentiometer is tied to this pin

// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 15.0f; //gain on the microphone
void setup() {
  //begin the serial comms (for debugging)
  audioHardware.beginBothSerial();  delay(500);
  BOTH_SERIAL.println("SoundLevelMeter: Starting setup()...");

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(20,audio_settings); 

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
    BOTH_SERIAL.println("Frequency Weighting: A_WEIGHT");
  } else {
    freqWeight1.setWeightingType(C_WEIGHT);        //A_WEIGHT or C_WEIGHT
    BOTH_SERIAL.println("Frequency Weighting: C_WEIGHT");
  }
  if (1) {
    calcLevel1.setTimeConst_sec(TIME_CONST_SLOW); //TIME_CONST_SLOW or TIME_CONST_FAST or use a value (seconds)
    BOTH_SERIAL.println("Time Weighting: SLOW");
  } else {
    calcLevel1.setTimeConst_sec(TIME_CONST_FAST); //TIME_CONST_SLOW or TIME_CONST_FAST or use a value (seconds)
    BOTH_SERIAL.println("Time Weighting: FAST");
  }

  // check the volume knob
  //servicePotentiometer(millis(),0);  //the "0" is not relevant here.

  BOTH_SERIAL.println("Setup complete.");
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //service record queue
  serviceAudioQueue();

  //printing of sound level
  printCurrentAndMaxSPL(millis(),500);  //print a value every 500 msec

  //check the potentiometer
  //servicePotentiometer(millis(),100); //service the potentiometer every 100 msec

  //check to see whether to print the CPU and Memory Usage
  //printCPUandMemory(millis(),3000); //print every 3000 msec

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
const float32_t SPL_scale_fac = 1.0/(20.e-6 * 20.e-6);
void printCurrentAndMaxSPL(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;
  if (cur_audio_pow < 0.0f) return;
  
  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    float32_t cur_SPL_dB = 10.0f*log10f(cur_audio_pow * SPL_scale_fac);
    float32_t max_SPL_dB = 10.0f*log10f(max_audio_pow * SPL_scale_fac);
    BOTH_SERIAL.print(cur_SPL_dB);
    BOTH_SERIAL.print(", ");
    BOTH_SERIAL.print(max_SPL_dB);
    BOTH_SERIAL.println();

    max_audio_pow = 0.0;  //recent for next block
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}

//servicePotentiometer: listens to the blue potentiometer and sends the new pot value
//  to the audio processing algorithm as a control parameter
void servicePotentiometer(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 100; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;
  static float prev_val = -1.0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    //read potentiometer
    float val = float(audioHardware.readPotentiometer()) / 1023.0; //0.0 to 1.0
    val = (1.0/9.0) * (float)((int)(9.0 * val + 0.5)); //quantize so that it doesn't chatter...0 to 1.0

    //send the potentiometer value to your algorithm as a control parameter
    if (abs(val - prev_val) > 0.05) { //is it different than before?
      prev_val = val;  //save the value for comparison for the next time around

      // //choose the desired gain value based on the knob setting
      //const float min_gain_dB = -20.0, max_gain_dB = 40.0; //set desired gain range
      //vol_knob_gain_dB = min_gain_dB + (max_gain_dB - min_gain_dB)*val; //computed desired gain value in dB

      // //command the new gain setting
      //gain1.setGain_dB(vol_knob_gain_dB);  //set the gain of the Left-channel gain processor
      //gain2.setGain_dB(vol_knob_gain_dB);  //set the gain of the Right-channel gain processor
      //BOTH_SERIAL.print("servicePotentiometer: Digital Gain dB = "); BOTH_SERIAL.println(vol_knob_gain_dB); //print text to Serial port for debugging
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
