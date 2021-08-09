/*
*   TrebleBoost_wApp_Alt
*
*   Created: Chip Audette, OpenAudio, August 2021
*   Purpose: Process audio by applying a high-pass filter followed by gain.  Includes App interaction.
*
*   TympanRemote App: https://play.google.com/store/apps/details?id=com.creare.tympanRemote
*
*   As a tutorial on how to interact with the mobile phone App, you should first explore the previous
*   example, which was "TrebleBoost_wApp".
*
*   Here in "TrebleBoost_wApp_Alt", we changed none of the signal processing of "TrebleBoost_wApp".
*      Instead, we just refactored the code to separate the different pieces of code into different
*      files so that it is easier to keep the pieces of code separate.  This aids in readability
*      and encapsulation.  By separating the code into separate files it makes it possible to create
*      much more complex programs than would be possible with everything lumped together in one file.
*
*   Like any App-enabled sketch, this example shows you:
*      * How to use Tympan code here to define a graphical interface (GUI) in the App
*      * How to transmit that GUI to the App
*      * How to respond to the commands coming back from the App
*
*   Compared to the previous example, this sketch:
*      * Moves the tracking of the programs settings into State.h
*      * Moves the interaction with the App (and USB Serial link) to SerialManager.h
*      
*  See the comments at the top of SerialManager.h and State.h for more description.
*
*   MIT License.  use at your own risk.
*/

//here are the libraries that we need
#include <Tympan_Library.h>  //include the Tympan Library
#include "SerialManager.h"
#include "State.h"

//set the sample rate and block size
const float sample_rate_Hz = 44100.0f ; //24000 or 44100 (or 44117, or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 32;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
Tympan                    myTympan(TympanRev::E,audio_settings);     //do TympanRev::D or TympanRev::E
AudioInputI2S_F32         i2s_in(audio_settings);     //Digital audio in *from* the Teensy Audio Board ADC.
AudioFilterBiquad_F32     hp_filt1(audio_settings);   //IIR filter doing a highpass filter.  Left.
AudioFilterBiquad_F32     hp_filt2(audio_settings);   //IIR filter doing a highpass filter.  Right.
AudioEffectGain_F32       gain1;                      //Applies digital gain to audio data.  Left.
AudioEffectGain_F32       gain2;                      //Applies digital gain to audio data.  Right.
AudioOutputI2S_F32        i2s_out(audio_settings);    //Digital audio out *to* the Teensy Audio Board DAC.

//Make all of the audio connections
AudioConnection_F32       patchCord1(i2s_in, 0, hp_filt1, 0);   //connect the Left input
AudioConnection_F32       patchCord2(i2s_in, 1, hp_filt2, 0);   //connect the Right input
AudioConnection_F32       patchCord3(hp_filt1, 0, gain1, 0);    //Left
AudioConnection_F32       patchCord4(hp_filt2, 0, gain2, 0);    //right
AudioConnection_F32       patchCord5(gain1, 0, i2s_out, 0);     //connect the Left gain to the Left output
AudioConnection_F32       patchCord6(gain2, 0, i2s_out, 1);     //connect the Right gain to the Right output

// Create classes for controlling the system
#include      "SerialManager.h"
#include      "State.h"                            
BLE           ble(&Serial1);                       //create bluetooth BLE
SerialManager serialManager(&ble);                 //create the serial manager for real-time control (via USB or App)
State         myState(&audio_settings, &myTympan, &serialManager); //keeping one's state is useful for the App's GUI


// define the setup() function, the function that is called once when the device is booting
void setup() {
  //begin the serial comms (for debugging)
  myTympan.beginBothSerial(); delay(1000);
  Serial.println("TrebleBoost_wApp: Starting setup()...");

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(10,audio_settings); 

  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate AIC

  //Choose the desired input
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  myTympan.volume_dB(myState.output_gain_dB);          // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  myTympan.setInputGain_dB(myState.input_gain_dB);     // set input volume, 0-47.5dB in 0.5dB setps

  //setup BLE
  while (Serial1.available()) Serial1.read(); //clear the incoming Serial1 (BT) buffer
  ble.setupBLE(myTympan.getBTFirmwareRev());  //this uses the default firmware assumption. You can override!

  //Set the cutoff frequency for the highpassfilter
  setHighpassFilters_Hz(myState.cutoff_Hz);   //function defined near the bottom of this file
  printHighpassCutoff();

  Serial.println("Setup complete.");
  serialManager.printHelp();
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //look for in-coming serial messages (via USB or via Bluetooth)
  if (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial

  //respond to BLE
  if (ble.available() > 0) {
    String msgFromBle; int msgLen = ble.recvBLE(&msgFromBle);
    for (int i=0; i < msgLen; i++) serialManager.respondToByte(msgFromBle[i]);
  }

  //service the BLE advertising state
  ble.updateAdvertising(millis(),5000); //check every 5000 msec to ensure it is advertising (if not connected)

  //periodically print the CPU and Memory Usage
  if (myState.printCPUtoGUI) {
    myTympan.printCPUandMemory(millis(),3000); //print every 3000 msec
    serviceUpdateCPUtoGUI(millis(),3000);      //print every 3000 msec
  }

} //end loop();


// ///////////////// Servicing routines


//Test to see if enough time has passed to send up updated CPU value to the App
void serviceUpdateCPUtoGUI(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    //send the latest value to the GUI!
    serialManager.updateCpuDisplayUsage();
    
    lastUpdate_millis = curTime_millis;
  } // end if
} //end serviceUpdateCPUtoGUI();



// ///////////////// functions used to respond to the commands

//change the gain from the App
void changeGain(float change_in_gain_dB) {
  myState.digital_gain_dB = myState.digital_gain_dB + change_in_gain_dB;
  gain1.setGain_dB(myState.digital_gain_dB);  //set the gain of the Left-channel gain processor
  gain2.setGain_dB(myState.digital_gain_dB);  //set the gain of the Right-channel gain processor
}

//Print gain levels 
void printGainLevels(void) {
  Serial.print("Analog Input Gain (dB) = "); 
  Serial.println(myState.input_gain_dB); //print text to Serial port for debugging
  Serial.print("Digital Gain (dB) = "); 
  Serial.println(myState.digital_gain_dB); //print text to Serial port for debugging
}

//change the highpass cutoff frequency
const float min_allowed_Hz = 100.0f;  //minimum allowed cutoff frequency (Hz)
const float max_allowed_Hz = 0.9 * (sample_rate_Hz/2.0);  //maximum allowed cutoff frequency (Hz);
void incrementHighpassFilters(float scaleFactor) { setHighpassFilters_Hz(myState.cutoff_Hz*scaleFactor); }
float setHighpassFilters_Hz(float freq_Hz) {
  myState.cutoff_Hz = min(max(freq_Hz,min_allowed_Hz),max_allowed_Hz); //set the new value as the new state
  hp_filt1.setHighpass(0, myState.cutoff_Hz); //biquad IIR filter.  left channel
  hp_filt2.setHighpass(0, myState.cutoff_Hz); //biquad IIR filter.  right channel
  return myState.cutoff_Hz;
}

void printHighpassCutoff(void) {
  Serial.println("Highpass filter cutoff at " + String(myState.cutoff_Hz,0) + " Hz");
}
