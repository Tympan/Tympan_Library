
/*
*   TrebleBoost_wComp_wApp
*
*   Created: Chip Audette, OpenAudio, August 2021
*   Purpose: Process audio by applying a high-pass filter followed by a WDRC compressor.  
*            Uses Tympan Earpieces.  Includes App interaction. 
*
*   TympanRemote App: https://play.google.com/store/apps/details?id=com.creare.tympanRemote
*
*   As a tutorial on how to interact with the mobile phone App, you should first explore the App Tutorial
*   example "TrebleBoost_wComp_wApp", which showed how we were using the pre-written App GUI for changing
*   all the settings in the compressor
*
*   Here in "TrebleBoost_wComp_wEarpieces_wApp", we started with the example sketch "TrebleBoost_wComp_wApp" 
*   and added the earpiece functionality.  The earpieces also have built-in pre-written App GUI elements
*   that we'll be using here.  (Oh, and I also added SD writing of the raw audio)
*
*   Like any App-enabled sketch, this example shows you:
*      * How to use Tympan code here to define a graphical interface (GUI) in the App
*      * How to transmit that GUI to the App
*      * How to respond to the commands coming back from the App
*
*   Compared to the previous example, this sketch:
*      * Adds the earpieces and their GUI elements
*      * Adds SD writing of the raw audio from the mics
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
Tympan                      myTympan(TympanRev::E, audio_settings);        //do TympanRev::D or TympanRev::E
EarpieceShield              earpieceShield(TympanRev::E, AICShieldRev::A); //in the Tympan_Library, EarpieceShield is defined in AICShield.h

//Create all the audio classes and make all of the audio connections
#include      "AudioConnections.h"

// Create classes for controlling the system
#include      "SerialManager.h"
#include      "State.h"                            
BLE           ble(&Serial1);                       //create bluetooth BLE
SerialManager serialManager(&ble);                 //create the serial manager for real-time control (via USB or App)
State         myState(&audio_settings, &myTympan, &serialManager); //keeping one's state is useful for the App's GUI


//define a function to configure the left and right compressors
void setupMyCompressors(void) {
  //set the speed of the compressor's response
  float attack_msec = 5.0;
  float release_msec = 300.0;
  comp1.setAttack_msec(attack_msec); //left channel
  comp2.setAttack_msec(attack_msec); //right channel
  comp1.setRelease_msec(release_msec); //left channel
  comp2.setRelease_msec(release_msec); //right channel

  //Single point system calibration.  what is the SPL of a full scale input (including effect of input_gain_dB)?
  float SPL_of_full_scale_input = 115.0;
  comp1.setMaxdB(SPL_of_full_scale_input);  //left channel
  comp2.setMaxdB(SPL_of_full_scale_input);  //right channel

  //set the linear gain
  float linear_gain_dB = 10.0;
  comp1.setGain_dB(linear_gain_dB);  //set the gain of the Left-channel linear gain at start of compression
  comp2.setGain_dB(linear_gain_dB);  //set the gain of the Right-channel linear gain at start of compression

  //now define the compression parameters
  float knee_compressor_dBSPL = 55.0;  //follows setMaxdB() above
  float comp_ratio = 1.5;
  comp1.setKneeCompressor_dBSPL(knee_compressor_dBSPL);  //left channel
  comp2.setKneeCompressor_dBSPL(knee_compressor_dBSPL);  //right channel
  comp1.setCompRatio(comp_ratio); //left channel
  comp2.setCompRatio(comp_ratio); //right channel

  //finally, the WDRC module includes a limiter at the end.  set its parameters
  float knee_limiter_dBSPL = SPL_of_full_scale_input - 20.0;  //follows setMaxdB() above
  //note: the WDRC limiter is hard-wired to a compression ratio of 10:1
  comp1.setKneeLimiter_dBSPL(knee_limiter_dBSPL);  //left channel
  comp2.setKneeLimiter_dBSPL(knee_limiter_dBSPL);  //right channel

}

//We're going to use a lot of built-in functionality of the compressor UI classes,
//including their built-in encapsulation of state and settings.  Let's connect that
//built-in stuff to our global State class via the function below.
void connectClassesToOverallState(void) {
  myState.comp1 = &comp1.state;
  myState.comp2 = &comp2.state;
  myState.earpieceMixer = &earpieceMixer.state;
  
}

//Again, we're going to use a lot of built-in functionality of the compressor UI classes,
//including their built-in encapsulation of serial responses and App GUI.  Let's connect
//that built-in stuff to our serial manager via the function below.  Once this is done,
//most of the GUI stuff will happen in the background without additional effort!
void setupSerialManager(void) {
  //register all the UI elements here
  serialManager.add_UI_element(&myState); //myState itself has some UI stuff we can use (like the CPU reporting!)
  serialManager.add_UI_element(&earpieceMixer);
  serialManager.add_UI_element(&comp1);   //The left compressor
  serialManager.add_UI_element(&comp2);   //The right compressor
  serialManager.add_UI_element(&audioSDWriter);
}

// define the setup() function, the function that is called once when the device is booting
void setup() {
  //begin the serial comms (for debugging)
  myTympan.beginBothSerial(); delay(1000);
  Serial.println("TrebleBoost_wComp_wEarpieces_wApp: Starting setup()...");

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(20,audio_settings); 

  //Enable the Tympan to start the audio flowing!
  myTympan.enable();       // activate the AIC on the main Tympan board
  earpieceShield.enable(); // activate the AIC on the earpiece shield
  earpieceMixer.setTympanAndShield(&myTympan, &earpieceShield); //the earpiece mixer must interact with the hardware, so point it to the hardware
  
  //setup the overall state and the serial manager
  connectClassesToOverallState();
  setupSerialManager();

  //Choose the default input
  if (1) {
    //default to the digital PDM mics within the Tympan earpieces
    earpieceMixer.setAnalogInputSource(EarpieceMixerState::INPUT_PCBMICS);  //Choose the desired audio analog input on the Typman...this will be overridden by the serviceMicDetect() in loop(), if micDetect is enabled
    earpieceMixer.setInputAnalogVsPDM(EarpieceMixerState::INPUT_PDM);       // ****but*** then activate the PDM mics
    Serial.println("setup: PDM Earpiece is the active input.");
  } else {
    //default to an analog input
    earpieceMixer.setAnalogInputSource(EarpieceMixerState::INPUT_MICJACK_MIC);  //Choose the desired audio analog input on the Typman...this will be overridden by the serviceMicDetect() in loop(), if micDetect is enabled
    Serial.println("setup: analog input is the active input.");
  }
  
  //Set the Bluetooth audio to go straight to the headphone amp, not through the Tympan software
  myTympan.mixBTAudioWithOutput(true);

  //set volumes
  myTympan.volume_dB(myState.output_gain_dB);  // -63.6 to +24 dB in 0.5dB steps.  uses signed 8-bit
  myTympan.setInputGain_dB(myState.earpieceMixer->inputGain_dB); // set MICPGA volume, 0-47.5dB in 0.5dB setps

  //set the highpass filter on the Tympan hardware to reduce DC drift
  float hardware_cutoff_Hz = 40.0;  //set the default cutoff frequency for the highpass filter
  myTympan.setHPFonADC(true, hardware_cutoff_Hz, audio_settings.sample_rate_Hz); //set to false to disable
  earpieceShield.setHPFonADC(true, hardware_cutoff_Hz, audio_settings.sample_rate_Hz); //set to false to disable
  
  //setup BLE
  while (Serial1.available()) Serial1.read(); //clear the incoming Serial1 (BT) buffer
  ble.setupBLE(myTympan.getBTFirmwareRev());  //this uses the default firmware assumption. You can override!

  //Set the cutoff frequency for the highpassfilter
  setHighpassFilters_Hz(myState.cutoff_Hz);   //function defined near the bottom of this file
  printHighpassCutoff();

  //configure the left and right compressors with the desired settings
  setupMyCompressors();

  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);
  audioSDWriter.setNumWriteChannels(4);     //can record 2 or 4 channels
  Serial.println("Setup(): SD configured for " + String(audioSDWriter.getNumWriteChannels()) + " channels.");

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

  //service the SD recording
  audioSDWriter.serviceSD_withWarnings(i2s_in); //For the warnings, it asks the i2s_in class for some info

  //service the LEDs...blink slow normally, blink fast if recording
  myTympan.serviceLEDs(millis(),audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING); 

  //periodically print the CPU and Memory Usage
  if (myState.flag_printCPUandMemory) myState.printCPUandMemory(millis(), 3000); //print every 3000msec  (method is built into TympanStateBase.h, which myState inherits from)
  if (myState.flag_printCPUandMemory) myState.printCPUtoGUI(millis(), 3000);     //send to App every 3000msec (method is built into TympanStateBase.h, which myState inherits from)

  //periodically print the gain status
  if (myState.flag_printSignalLevels) printSignalLevels(millis(),1000); //update every 1000 msec

} //end loop();


// ///////////////// Servicing routines

//This routine plots the input and output level of the left compressor
void printSignalLevels(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;
  
  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    
    float in_dB = comp1.getCurrentLevel_dB();         //here is what the compressor is seeing at its input
    float gain_dB = comp1.getCurrentGain_dB();
    float out_dB = in_dB + gain_dB; //this is the ouptut, including the effect of the changing gain of the compressor
    
    myTympan.println("State: printLevels: Left In, gain, Out (dBFS) = " + String(in_dB,1) + ", " + String(gain_dB,1) + ", " + String(out_dB,1)); //send to USB Serial
    serialManager.sendDataToAppSerialPlotter(String(in_dB,1),String(out_dB,1));  //send to App (for plotting)
  
    lastUpdate_millis = curTime_millis;
  } // end if
}


// ///////////////// functions used to respond to the commands

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
