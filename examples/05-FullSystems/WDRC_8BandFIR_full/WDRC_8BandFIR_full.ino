/*
  WDRC_8BandFIR_full

  Created: Chip Audette (OpenAudio), Feb 2017 (Updated July 2021)
    Primarly built upon CHAPRO "Generic Hearing Aid" from
    Boys Town National Research Hospital (BTNRH): https://github.com/BTNRH/chapro

  Purpose: Implements 8-band WDRC compressor.  The BTNRH version was implemented the
    filters in the frequency-domain, whereas I implemented them as FIR filters
	in the time-domain. I've also added an expansion stage to manage noise at very
	low SPL.  Communicates via USB Serial and via Bluetooth Serial

  User Controls:
    Potentiometer on Tympan controls the algorithm gain

   MIT License.  use at your own risk.
*/

// Include all the of the needed libraries
#include <Tympan_Library.h>


// Define the audio settings
const float sample_rate_Hz = 24000.0f ; //24000 or 44117.64706f (or other frequencies in the table in AudioOutputI2S_F32
const int audio_block_samples = 16;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32   audio_settings(sample_rate_Hz, audio_block_samples);


// Create audio classes and make audio connections
Tympan    myTympan(TympanRev::E, audio_settings);  //choose TympanRev::D or TympanRev::E
const int N_CHAN = 8;                              // number of frequency bands (channels)
#include "AudioConnections.h"                      //let's put them in their own file for clarity


// Create classes for controlling the system
#include      "SerialManager.h"
#include      "State.h"                            //must be after N_CHAN is defined
BLE           ble(&Serial1);                       //create bluetooth BLE
SerialManager serialManager(&ble);                 //create the serial manager for real-time control (via USB or App)
State         myState(&audio_settings, &myTympan, &serialManager); //keeping one's state is useful for the App's GUI


//configure the parameters of the audio-processing classes (must be after myState is created)
#include "ConfigureAlgorithms.h" //let's put all of these functions in their own file for clarity


//function to setup the hardware
void setupTympanHardware(void) {
  Serial.println("Setting up Tympan Audio Board...");
  myTympan.enable(); // activate AIC

  //setup DC-blocking highpass filter running in the ADC hardware itself
  float cutoff_Hz = 40.0;  //set the default cutoff frequency for the highpass filter
  myTympan.setHPFonADC(true,cutoff_Hz,audio_settings.sample_rate_Hz); //set to false to disble

  //Choose the desired audio input on the Typman
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board micropphones
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack - defaults to mic bias 2.5V
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //set volumes
  setOutputGain_dB(0.f);  // -63.6 to +24 dB in 0.5dB steps.  uses signed 8-bit
  float default_mic_input_gain_dB = 15.0f; //gain on the microphone
  setInputGain_dB(default_mic_input_gain_dB); // set MICPGA volume, 0-47.5dB in 0.5dB setps
}


// ///////////////// Main setup() and loop() as required for all Arduino programs

// define the setup() function, the function that is called once when the device is booting
int USE_VOLUME_KNOB = 1;  //set to 1 to use volume knob to override the default vol_knob_gain_dB set a few lines below
void setup() {
  myTympan.beginBothSerial();
  Serial.println("WDRC_8BandFIR_Full: setup():...");
  Serial.print("Sample Rate (Hz): "); Serial.println(audio_settings.sample_rate_Hz);
  Serial.print("Audio Block Size (samples): "); Serial.println(audio_settings.audio_block_samples);

  // Audio connections require memory
  AudioMemory_F32(40,audio_settings);  //allocate Float32 audio data blocks (primary memory used for audio processing)

  // Enable the audio shield, select input, and enable output
  setupTympanHardware();

  //setup filters and mixers
  setupAudioProcessing();

  //update the potentiometer settings
	if (USE_VOLUME_KNOB) servicePotentiometer(millis());

  //setup BLE
  while (Serial1.available()) Serial1.read(); //clear the incoming Serial1 (BT) buffer  
  ble.setupBLE(myTympan.getBTFirmwareRev());  //this uses the default firmware assumption. You can override!
  
  //End of setup
  printGainSettings();
  Serial.println("Setup complete.");
  serialManager.printHelp();

} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //respond to Serial commands
  while (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB...respondToByte is in SerialManagerBase...it then calls SerialManager.processCharacter(c)

  //respond to BLE
  if (ble.available() > 0) {
    String msgFromBle; int msgLen = ble.recvBLE(&msgFromBle);
    for (int i=0; i < msgLen; i++) serialManager.respondToByte(msgFromBle[i]);  //respondToByte is in SerialManagerBase...it then calls SerialManager.processCharacter(c)
  }

  //service the BLE advertising state
   ble.updateAdvertising(millis(),5000); //check every 5000 msec to ensure it is advertising (if not connected)

  //service the potentiometer...if enough time has passed
  if (USE_VOLUME_KNOB) servicePotentiometer(millis());
  
  //periodically print the CPU and Memory Usage
  if (myState.flag_printCPUandMemory) myState.printCPUandMemory(millis(), 3000); //print every 3000msec  (method is built into TympanStateBase.h, which myState inherits from)
  if (myState.flag_printCPUandMemory) myState.printCPUtoGUI(millis(), 3000);     //send to App every 3000msec (method is built into TympanStateBase.h, which myState inherits from)

  //print info about the signal processing
  updateAveSignalLevels(millis());
  if (myState.enable_printAveSignalLevels) myState.printAveSignalLevels(millis(),3000);

} //end loop()


// ///////////////// Servicing routines

//servicePotentiometer: listens to the blue potentiometer and sends the new pot value
//  to the audio processing algorithm as a control parameter
void servicePotentiometer(unsigned long curTime_millis) {
  static unsigned long updatePeriod_millis = 100; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;
  static float prev_val = -1.0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    //read potentiometer
    float val = float(myTympan.readPotentiometer()) / 1023.0; //0.0 to 1.0
    val = (1.0/9.0) * (float)((int)(9.0 * val + 0.5)); //quantize so that it doesn't chatter...0 to 1.0

    //send the potentiometer value to your algorithm as a control parameter
    //float scaled_val = val / 3.0; scaled_val = scaled_val * scaled_val;
    if (abs(val - prev_val) > 0.05) { //is it different than befor?
      prev_val = val;  //save the value for comparison for the next time around
      setDigitalGain_dB(val*45.0f - 25.0f,false);
      Serial.print("servicePotentiometer: digital gain (dB) = ");Serial.println(myState.digital_gain_dB);
    }
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();



void updateAveSignalLevels(unsigned long curTime_millis) {
  static unsigned long updatePeriod_millis = 100; //how often to perform the averaging
  static unsigned long lastUpdate_millis = 0;
  float update_coeff = 0.2; //Smoothing filter.  Choose 1.0 (for no smoothing) down to 0.0 (which would never update)

  //is it time to update the calculations
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    for (int i=0; i<N_CHAN; i++) { //loop over each band
      myState.aveSignalLevels_dBFS[i] = (1.0f-update_coeff)*myState.aveSignalLevels_dBFS[i] + update_coeff*expCompLim[i].getCurrentLevel_dB(); //running average
    }
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}

// ////////////////////////////// Functions to set the parameters and maintain the state

// Control the System Gains
float setInputGain_dB(float gain_dB) { return myState.input_gain_dB = myTympan.setInputGain_dB(gain_dB); }
float setOutputGain_dB(float gain_dB) {  return myState.output_gain_dB = myTympan.volume_dB(gain_dB); }
float setDigitalGain_dB(float gain_dB) { return setDigitalGain_dB(gain_dB, true); }
float setDigitalGain_dB(float gain_dB, bool printToUSBSerial) {  
  myState.digital_gain_dB = broadbandGain.setGain_dB(gain_dB); 
  serialManager.updateGainDisplay();
  if (printToUSBSerial) printGainSettings();
  return myState.digital_gain_dB;
}
float incrementDigitalGain(float increment_dB) { return setDigitalGain_dB(myState.digital_gain_dB + increment_dB); }

void printGainSettings(void) {
  Serial.print("Gain (dB): ");
  Serial.print("Input PGA = "); Serial.print(myState.input_gain_dB,1);
  Serial.print(", Per-Channel = ");
  for (int i=0; i<N_CHAN; i++) {
    Serial.print(expCompLim[i].getGain_dB(),1);
    Serial.print(", ");
  }
  Serial.print("Knob = "); Serial.print(myState.digital_gain_dB,1);
  Serial.println();
}

// Control Printing of Signal Levels
void togglePrintAveSignalLevels(bool as_dBSPL) { 
  myState.enable_printAveSignalLevels = !myState.enable_printAveSignalLevels; 
  myState.printAveSignalLevels_as_dBSPL = as_dBSPL;
};
