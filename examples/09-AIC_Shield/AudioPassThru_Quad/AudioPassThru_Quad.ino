/*
 * AudioPassThru_Quad
 *
 * Created: Chip Audette, OpenAudio, May 2019
 * Purpose: Demonstrate basic audio pass-thru using the Tympan Audio Board
 *     with the AIC Shield
 *
 * Uses Teensy Audio default sample rate of 44100 Hz with Block Size of 128
 *
 * License: MIT License, Use At Your Own Risk
 *
 */

//here are the libraries that we need
#include <Tympan_Library.h>  //include the Tympan Library

//set the sample rate and block size
const float sample_rate_Hz = 44117.0f ;  //24000 to 44117 to 96000 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 128;     //do not make bigger than audio_block_SAMPLES from AudioStream.h (which is 128)  Must be 128 for SD recording.
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

// define classes to control the Tympan and the AIC_Shield
Tympan                        myTympan;    //note: Rev C is not compatible with the AIC shield
AICShield                     aicShield;   //note: Rev C is not compatible with the AIC shield

// define audio classes
AudioInputI2SQuad_F32         i2s_in(audio_settings);        //Digital audio *from* the Tympan AIC.
AudioOutputI2SQuad_F32        i2s_out(audio_settings);       //Digital audio *to* the Tympan AIC.  Always list last to minimize latency

// define audio connections
AudioConnection_F32           patchCord1(i2s_in, 0, i2s_out, 0); //connect first left input to first left output
AudioConnection_F32           patchCord2(i2s_in, 1, i2s_out, 1); //connect first right input to first right output
AudioConnection_F32           patchCord3(i2s_in, 2, i2s_out, 2); //connect second left input to second left output
AudioConnection_F32           patchCord4(i2s_in, 3, i2s_out, 3); //connect second right input to second right output

// define the setup() function, the function that is called once when the device is booting
void setup(void)
{
  //begin the serial comms (for debugging)
  myTympan.beginBothSerial();   delay(1000);
  Serial.println("AudioPassThru_Quad: Starting setup()...");

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(20,audio_settings); 

  //Enable the Tympan and AIC shields to start the audio flowing!
  myTympan.enable(); 
  aicShield.enable();

  //Choose the desired input for Tympan main board
  switch (1) {
    case 1:
      Serial.println("Main Board Using PCB Mics");
      myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones (only on main board, not on AIC shield)
      myTympan.setInputGain_dB(15.0); // set input volume, 0-47.5dB in 0.5dB setps
      break;
    case 2:
      Serial.println("Main Board Using Input Jack for Mics");
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);     // use the on board microphones (only on main board, not on AIC shield)
      myTympan.setInputGain_dB(15.0); // set input volume, 0-47.5dB in 0.5dB setps
      break;
    case 3:
      Serial.println("Main Board Using Input Jack for Line-In");
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF
      myTympan.setInputGain_dB(0.0); // set input volume, 0-47.5dB in 0.5dB setps
      break;
  }

  //Choose the desired input for AIC Shield
  switch (3) {
    //case 1:
    //  //AIC_Shield does NOT have PCB mics
    case 2:
      Serial.println("AIC Shield Using Input Jack for Mics");
      aicShield.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);     // use the on board microphones (only on main board, not on AIC shield)
      aicShield.setInputGain_dB(15.0); // set input volume, 0-47.5dB in 0.5dB setps
      break;
    case 3:
      Serial.println("AIC Shield Using Input Jack for Line-In");
      aicShield.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF
      aicShield.setInputGain_dB(0.0); // set input volume, 0-47.5dB in 0.5dB setps
      break;
  }
  
  //Set the desired volume levels
  myTympan.volume_dB(0);      // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  aicShield.volume_dB(0);     // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  
  Serial.println("Setup complete.");
  myTympan.setAmberLED(true);  //light up the LED just to show that setup is complete
}

void loop(void)
{
  // Nothing to do - just looping input to output
  delay(2000);
  printCPUandMemoryMessage();
}

void printCPUandMemoryMessage(void) {
    Serial.print("CPU Cur/Pk: ");
    Serial.print(audio_settings.processorUsage(),1);
    Serial.print("%/");
    Serial.print(audio_settings.processorUsageMax(),1);
    Serial.print("%, ");
    Serial.print("MEM Cur/Pk: ");
    Serial.print(AudioMemoryUsage_F32());
    Serial.print("/");
    Serial.print(AudioMemoryUsageMax_F32());
    Serial.println();
}
