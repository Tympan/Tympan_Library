/*
*   TrebleBoost_wComp
*
*   Created: Chip Audette, OpenAudio, Apr 2017
*   Purpose: Process audio by applying a high-pass filter followed by gain followed
*            by a dynamic range compressor.
*
*   Blue potentiometer adjusts the digital gain applied to the filtered audio signal.
*
*   MIT License.  use at your own risk.
*/

//here are the libraries that we need
#include <Tympan_Library.h>  //include the Tympan Library

//set the sample rate and block size
const float sample_rate_Hz = 24000.0f ; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 32;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
Tympan                    myTympan(TympanRev::D);     //do TympanRev::D or TympanRev::C
AudioInputI2S_F32         i2s_in(audio_settings);     //Digital audio in *from* the Teensy Audio Board ADC.
AudioFilterBiquad_F32     hp_filt1(audio_settings);   //IIR filter doing a highpass filter.  Left.
AudioFilterBiquad_F32     hp_filt2(audio_settings);   //IIR filter doing a highpass filter.  Right.
AudioEffectCompWDRC_F32   comp1(audio_settings);      //Compresses the dynamic range of the audio.  Left.
AudioEffectCompWDRC_F32   comp2(audio_settings);      //Compresses the dynamic range of the audio.  Right.
AudioOutputI2S_F32        i2s_out(audio_settings);    //Digital audio out *to* the Teensy Audio Board DAC.

//Make all of the audio connections
AudioConnection_F32       patchCord1(i2s_in, 0, hp_filt1, 0);   //connect the Left input to the left highpass filter
AudioConnection_F32       patchCord2(i2s_in, 1, hp_filt2, 0);   //connect the Right input to the right highpass filter
AudioConnection_F32       patchCord3(hp_filt1, 0, comp1, 0);    //connect to the left gain/compressor/limiter
AudioConnection_F32       patchCord4(hp_filt2, 0, comp2, 0);    //connect to the right gain/compressor/limiter
AudioConnection_F32       patchCord5(comp1, 0, i2s_out, 0);     //connectto the Left output
AudioConnection_F32       patchCord6(comp2, 0, i2s_out, 1);     //connect to the Right output

//define a function to configure the left and right compressors
void setupMyCompressors(void) {
  //set the speed of the compressor's response
  float attack_msec = 5.0;
  float release_msec = 300.0;
  comp1.setAttackRelease_msec(attack_msec, release_msec); //left channel
  comp2.setAttackRelease_msec(attack_msec, release_msec); //right channel

  //Single point system calibration.  what is the SPL of a full scale input (including effect of input_gain_dB)?
  float SPL_of_full_scale_input = 115.0;
  comp1.setMaxdB(SPL_of_full_scale_input);  //left channel
  comp2.setMaxdB(SPL_of_full_scale_input);  //right channel

  //now define the compression parameters
  float knee_compressor_dBSPL = 55.0;  //follows setMaxdB() above
  float comp_ratio = 1.5;
  comp1.setKneeCompressor_dBSPL(knee_compressor_dBSPL);  //left channel
  comp2.setKneeCompressor_dBSPL(knee_compressor_dBSPL);  //right channel
  comp1.setCompRatio(comp_ratio); //left channel
  comp2.setCompRatio(comp_ratio);

  //finally, the WDRC module includes a limiter at the end.  set its parameters
  float knee_limiter_dBSPL = SPL_of_full_scale_input - 20.0;  //follows setMaxdB() above
  //note: the WDRC limiter is hard-wired to a compression ratio of 10:1
  comp1.setKneeLimiter_dBSPL(knee_limiter_dBSPL);  //left channel
  comp2.setKneeLimiter_dBSPL(knee_limiter_dBSPL);  //right channel

}


// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 20.0f; //gain on the microphone
float vol_knob_gain_dB = 0.0;      //will be overridden by volume knob
void setup() {
  //begin the serial comms (for debugging)
  Serial.begin(115200);  delay(500);
  Serial.println("TrebleBoost_wComp: Starting setup()...");

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(10,audio_settings); 

  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate AIC

  //Choose the desired input
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  myTympan.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  myTympan.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  //Set the cutoff frequency for the highpassfilter
  float cutoff_Hz = 1000.f;  //frequencies below this will be attenuated
  Serial.print("Highpass filter cutoff at ");Serial.print(cutoff_Hz);Serial.println(" Hz");
  hp_filt1.setHighpass(0, cutoff_Hz); //biquad IIR filter.  left channel
  hp_filt2.setHighpass(0, cutoff_Hz); //biquad IIR filter.  right channel

  //configure the left and right compressors with the desired settings
  setupMyCompressors();

  // check the setting on the potentiometer
  servicePotentiometer(millis(),0);

  Serial.println("Setup complete.");
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //periodically check the potentiometer
  servicePotentiometer(millis(),100); //update every 100 msec

  //check to see whether to print the CPU and Memory Usage
  printCPUandMemory(millis(),3000); //print every 3000 msec

  //periodically print the gain status
  printGainStatus(millis(),2000); //update every 4000 msec

} //end loop();


// ///////////////// Servicing routines

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
    float val = float(myTympan.readPotentiometer()) / 1024.0; //0.0 to 1.0
    val = (1.0/9.0) * (float)((int)(9.0 * val + 0.5)); //quantize so that it doesn't chatter...0 to 1.0

    //send the potentiometer value to your algorithm as a control parameter
    if (abs(val - prev_val) > 0.05) { //is it different than before?
      prev_val = val;  //save the value for comparison for the next time around

      //choose the desired gain value based on the knob setting
      const float min_gain_dB = -20.0, max_gain_dB = 40.0; //set desired gain range
      vol_knob_gain_dB = min_gain_dB + (max_gain_dB - min_gain_dB)*val; //computed desired gain value in dB

      //command the new gain setting
      comp1.setGain_dB(vol_knob_gain_dB);  //set the gain of the Left-channel linear gain at start of compression
      comp2.setGain_dB(vol_knob_gain_dB);  //set the gain of the Right-channel linear gain at start of compression
      Serial.print("servicePotentiometer: linear gain dB = "); Serial.println(vol_knob_gain_dB); //print text to Serial port for debugging
    }
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();


void printCPUandMemory(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 3000; //how many milliseconds between updating gain reading?
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    printCPUandMemoryMessage();
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}

extern "C" char* sbrk(int incr);
int FreeRam() {
  char top; //this new variable is, in effect, the mem location of the edge of the heap
  return &top - reinterpret_cast<char*>(sbrk(0));
}
void printCPUandMemoryMessage(void) {
  BOTH_SERIAL.print("CPU Cur/Pk: ");
  BOTH_SERIAL.print(audio_settings.processorUsage(), 1);
  BOTH_SERIAL.print("%/");
  BOTH_SERIAL.print(audio_settings.processorUsageMax(), 1);
  BOTH_SERIAL.print("%, ");
  BOTH_SERIAL.print("MEM Cur/Pk: ");
  BOTH_SERIAL.print(AudioMemoryUsage_F32());
  BOTH_SERIAL.print("/");
  BOTH_SERIAL.print(AudioMemoryUsageMax_F32());
  BOTH_SERIAL.print(", FreeRAM(B) ");
  BOTH_SERIAL.print(FreeRam());
  BOTH_SERIAL.println();
}



//This routine plots the current gain settings, including the dynamically changing gains
//of the compressors
void printGainStatus(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 2000; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    Serial.print("printGainStatus: ");

    Serial.print("Input PGA = ");
    Serial.print(input_gain_dB,1);
    Serial.print(" dB.");

    Serial.print(" Compressor Gain (L/R) = ");
    Serial.print(comp1.getCurrentGain_dB(),1);
    Serial.print(", ");
    Serial.print(comp2.getCurrentGain_dB(),1);
    Serial.print(" dB.");

    Serial.println();

    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();
