/*
*   TestDynamicAudioInstances
*
*   Created: Chip Audette, OpenAudio, Aug 2024
*   Purpose: Dynamically switches audio between two different audio paths by creating
*      and destroying audio objects and audio connections.
*
*   Use the serial monitor to command the switching.
*
*   MIT License.  use at your own risk.
*/

//here are the libraries that we need
#include <Tympan_Library.h>  //include the Tympan Library
#include "AudioPaths.h"
#include "SerialManager.h"

//set the sample rate and block size
const float sample_rate_Hz = 16000.0f ; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 64;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
Tympan                  myTympan(TympanRev::E, audio_settings);   //do TympanRev::D or E or F
AudioInputI2S_F32       audioInput(audio_settings);
AudioPath_Base          *currentAudioPath = NULL;
AudioOutputI2S_F32      audioOutput(audio_settings);

//control display and serial interaction
bool enable_printCPUandMemory = false;
void togglePrintMemoryAndCPU(void) { enable_printCPUandMemory = !enable_printCPUandMemory; };
SerialManager serialManager;

// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 5.0f; //gain on the microphone
void setup() {
  //begin the serial comms (for debugging)
  Serial.println("TestDynamicConnections: Starting setup()...");

  //Instantiate the default AudioPath that we desire
  currentAudioPath = new AudioPath_Sine(audio_settings, &audioInput, &audioOutput);

  //allocate the audio memory
  AudioMemory(10);  //is this needed?
  AudioMemory_F32(100,audio_settings); //allocate memory

  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate AIC

  //Choose the desired input
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on board microphones
  // myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack - defaults to mic bias 2.5V
  // myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  myTympan.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  myTympan.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  //End of setup
  Serial.println("Setup complete.");
  serialManager.printHelp();
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //respond to Serial commands, if any have been received
  while (Serial.available()) serialManager.respondToByte((char)Serial.read());

  //update the memory and CPU usage...if enough time has passed
  if (enable_printCPUandMemory) myTympan.printCPUandMemory(millis(),500); //update every 3000 msec

  //service any main-loop needs of the AudioPath object
  if (currentAudioPath != NULL) currentAudioPath->serviceMainLoop();

} //end loop();


