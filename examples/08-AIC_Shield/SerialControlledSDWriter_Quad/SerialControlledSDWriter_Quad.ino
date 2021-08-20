/*
   SerialControlledSDWriter_Quad
   
   Created: Chip Audette, OpenAudio, May 2019 (Updated Aug 2021)
   Purpose: Write 4 channels of audio to SD based on serial commands
      via USB serial or via BT serial.
	  
	  Writes to a single 4-channel WAV file.  Can be opened in Audacity.

   MIT License.  use at your own risk.
*/


// Include all the of the needed libraries
#include <Tympan_Library.h>

//define state...the "myState" instance is actually created in SerialManager.h
#define NO_STATE (-1)
const int INPUT_PCBMICS = 0, INPUT_MICJACK = 1, INPUT_LINEIN_SE = 2, INPUT_LINEIN_JACK = 3;
class State_t {
  public:
    int input_source = NO_STATE;
};

//local files
#include "SerialManager.h"

//set the sample rate and block size
const float sample_rate_Hz = 44117.0f ; //24000 or 44117 or 96000 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 64;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)  Must be 128 for SD recording.
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

// Define the overall setup
String overall_name = String("Tympan: Quad SD Audio Writer");
float default_input_gain_dB = 5.0f; //gain on the microphone
float input_gain_dB = default_input_gain_dB;
#define MAX_AUDIO_MEM 60

// /////////// Define audio objects...they are configured later

// define classes to control the Tympan and the AIC_Shield
Tympan                        myTympan(TympanRev::E, audio_settings);    //choose TympanRev::D or TympanRev::E
AICShield                     aicShield(TympanRev::E, AICShieldRev::A);  //choose TympanRev::D or TympanRev::E 
AudioInputI2SQuad_F32         i2s_in(audio_settings);   //Digital audio input from the ADC
AudioSDWriter_F32             audioSDWriter(audio_settings); //this is stereo by default
AudioOutputI2SQuad_F32        i2s_out(audio_settings);  //Digital audio output to the DAC.  Should always be last.

//Connect to outputs
AudioConnection_F32           patchcord1(i2s_in, 0, i2s_out, 0);    //Left channel input to left channel output
AudioConnection_F32           patchcord2(i2s_in, 1, i2s_out, 1);    //Right channel input to right channel output
AudioConnection_F32           patchcord3(i2s_in, 2, i2s_out, 2);    //AIC Shield Left channel input to AIC Shield left channel output
AudioConnection_F32           patchcord4(i2s_in, 3, i2s_out, 3);    //AIC Shield Left channel input to AIC Shield right channel output

//Connect to SD logging
AudioConnection_F32           patchcord5(i2s_in, 0, audioSDWriter, 0);   //connect Raw audio to SD writer
AudioConnection_F32           patchcord6(i2s_in, 1, audioSDWriter, 1);   //connect Raw audio to SD writer
AudioConnection_F32           patchcord7(i2s_in, 2, audioSDWriter, 2);   //connect Raw audio to SD writer
AudioConnection_F32           patchcord8(i2s_in, 3, audioSDWriter, 3);   //connect Raw audio to SD writer


//control display and serial interaction
SerialManager serialManager;


//keep track of state
State_t myState;

void setConfiguration(int config) {
  myState.input_source = config;

  switch (config) {
    case INPUT_PCBMICS:
      //Select Input and set gain
      myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones
      aicShield.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones..the AIC sheild doesn't have on-board mics, so you probably want the line below
      //aicShield.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // AIC sheild doesn't have PCB mics, so force to input jack
      input_gain_dB = 15;
      myTympan.setInputGain_dB(input_gain_dB); aicShield.setInputGain_dB(input_gain_dB);
      break;

    case INPUT_MICJACK:
      //Select Input and set gain
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the mic jack
      aicShield.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the mic jack
      myTympan.setEnableStereoExtMicBias(true);  //put the mic bias on both channels...does this also do the aicShield?
      //aicShield.setEnableStereoExtMicBias(true);  //put the mic bias on both channels
      input_gain_dB = default_input_gain_dB;
      myTympan.setInputGain_dB(input_gain_dB);
      aicShield.setInputGain_dB(input_gain_dB);
      break;
      
    case INPUT_LINEIN_JACK:
      //Select Input and set gain
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the line-input through holes
      aicShield.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the line-input through holes
      input_gain_dB = 0;
      myTympan.setInputGain_dB(input_gain_dB);
      aicShield.setInputGain_dB(input_gain_dB);
      break;
      
    case INPUT_LINEIN_SE:
      //Select Input and set gain
      myTympan.inputSelect(TYMPAN_INPUT_LINE_IN); // use the line-input through holes
      aicShield.inputSelect(TYMPAN_INPUT_LINE_IN); // use the line-input through holes
      input_gain_dB = default_input_gain_dB;
      myTympan.setInputGain_dB(input_gain_dB);
      aicShield.setInputGain_dB(input_gain_dB);
      break;
  }
 }

// ///////////////// Main setup() and loop() as required for all Arduino programs

// define the setup() function, the function that is called once when the device is booting
void setup() {
  delay(100);
  myTympan.beginBothSerial(); delay(1000);
  myTympan.print(overall_name); myTympan.println(": setup():...");
  myTympan.print("Sample Rate (Hz): "); myTympan.println(audio_settings.sample_rate_Hz);
  myTympan.print("Audio Block Size (samples): "); myTympan.println(audio_settings.audio_block_samples);

  //allocate the dynamically re-allocatable audio memory
  AudioMemory_F32(MAX_AUDIO_MEM, audio_settings); 

  //activate the Tympan audio hardware
  myTympan.enable();   aicShield.enable();     // activate AIC
  myTympan.volume_dB(0.0);  aicShield.volume_dB(0); // output amp: -63.6 to +24 dB in 0.5dB steps.  uses signed 8-bit

  //Configure for Tympan PCB mics
  myTympan.println("Setup: Using Mic Jack with Mic Bias.");
  setConfiguration(INPUT_MICJACK); //this will also unmute the system

  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);
  audioSDWriter.setNumWriteChannels(4);             //four channels for this quad recorder, but you could set it to 2
  audioSDWriter.setWriteDataType(AudioSDWriter::WriteDataType::INT16);  //this is the built-in the default, but here you could change it to FLOAT32
  myTympan.print("Configured for "); myTympan.print(audioSDWriter.getNumWriteChannels()); myTympan.println(" channels to SD.");

  //End of setup
  myTympan.println("Setup: complete."); serialManager.printHelp();

} //end setup()

// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //respond to Serial commands
  if (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial
  //if (Serial1.available()) serialManager.respondToByte((char)Serial1.read()); //BT Serial

  //service the SD recording
  audioSDWriter.serviceSD_withWarnings(i2s_in); //For the warnings, it asks the i2s_in class for some info

  //service the LEDs...blink slow normally, blink fast if recording
  myTympan.serviceLEDs(millis(),audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING); 


} //end loop()




// //////////////////////////////////// Control the audio processing from the SerialManager
//here's a function to change the volume settings.   We'll also invoke it from our serialManager
void incrementInputGain(float increment_dB) {
  input_gain_dB += increment_dB;
  if (input_gain_dB < 0.0) {
    myTympan.println("Error: cannot set input gain less than 0 dB.");
    myTympan.println("Setting input gain to 0 dB.");
    input_gain_dB = 0.0;
  }
  myTympan.setInputGain_dB(input_gain_dB);
}


