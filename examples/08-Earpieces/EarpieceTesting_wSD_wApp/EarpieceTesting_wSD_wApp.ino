/*
   EarpieceTesting_wSD_wApp
   
   Created: Chip Audette, OpenHearing, May 2024
   Purpose: Enable testing of input and output of earpieces
      * Uses Tympan with Earpiece Shield
      * Generate steady tones
        - Vary frequency and loudness from serial or from App 
      * Record audio from earpieces to SD card
         - Access SD card from PC via MTP

   For Tympan Rev D, program in Arduino IDE as a Teensy 3.6.
   For Tympan Rev E/F, program in Arduino IDE as a Teensy 4.1.

   MIT License.  use at your own risk.
*/


// Include all the of the needed libraries
#include <Tympan_Library.h>

//set the sample rate and block size
const float sample_rate_Hz = 44100.0f ;  //Desired sample rate
const int audio_block_samples = 32;     //Number of samples per audio block (do not make bigger than 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);


// /////////// Define audio objects

// define classes to control the Tympan and the Earpiece shield
Tympan           myTympan(TympanRev::F, audio_settings);                   //do TympanRev::D or E or F
EarpieceShield   earpieceShield(myTympan.getTympanRev(), AICShieldRev::A); //in the Tympan_Library, EarpieceShield is defined in AICShield.h

//create audio library objects for handling the audio
#include "AudioConnections.h"

// Create classes for controlling the system
#include      "SerialManager.h"
#include      "State.h"                            
BLE_UI& ble = myTympan.getBLE_UI();         //myTympan owns the ble object, but we have a reference to it here
SerialManager serialManager(&ble);    //create the serial manager for real-time control (via USB or App)
State         myState(&audio_settings, &myTympan, &serialManager); //keeping one's state is useful for the App's GUI

// Create the MTP servicing stuff so that one can access the SD card via USB
// If you want this, be sure to set the USB mode via the Arduino IDE,  Tools Menu -> USB Type -> Serial + MTP (experimental)
#include "setup_MTP.h"  //put this line sometime after the audioSDWriter has been instantiated

//We're going to use a lot of built-in functionality of the compressor UI classes,
//including their built-in encapsulation of state and settings.  Let's connect that
//built-in stuff to our global State class via the function below.
void connectClassesToOverallState(void) {
  myState.earpieceMixer = &earpieceMixer.state;
  myState.sineTone = &sineTone.state;
  
}


//Again, we're going to use a lot of built-in functionality of the compressor UI classes,
//including their built-in encapsulation of serial responses and App GUI.  Let's connect
//that built-in stuff to our serial manager via the function below.  Once this is done,
//most of the GUI stuff will happen in the background without additional effort!
void setupSerialManager(void) {
  //register all the UI elements here
  //serialManager.add_UI_element(&earpieceMixer);
  serialManager.add_UI_element(&sineTone);
  serialManager.add_UI_element(&audioSDWriter);
  serialManager.add_UI_element(&myState); //myState itself has some UI stuff we can use (like the CPU reporting!)
  
}


// ///////////////// Main setup() and loop() as required for all Arduino programs

// define the setup() function, the function that is called once when the device is booting
void setup() {
  myTympan.beginBothSerial(); delay(1500);
  Serial.println("EarpieceTesting_wSD_wApp: setup():...");
  Serial.println("Sample Rate (Hz): " + String(audio_settings.sample_rate_Hz));
  Serial.println("Audio Block Size (samples): " + String(audio_settings.audio_block_samples));

  //allocate the dynamically re-allocatable audio memory
  AudioMemory_F32(100, audio_settings); 

  //Enable the Tympan and earpiece shields to start the audio flowing!
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

  //set volumes
  myTympan.volume_dB(myState.output_dacGain_dB);  // -63.6 to +24 dB in 0.5dB steps.  uses signed 8-bit
  myTympan.setInputGain_dB(myState.earpieceMixer->inputGain_dB); // set MICPGA volume, 0-47.5dB in 0.5dB setps

  //set the highpass filter on the Tympan hardware to reduce DC drift
  float hardware_cutoff_Hz = 40.0;  //set the default cutoff frequency for the highpass filter
  myTympan.setHPFonADC(true, hardware_cutoff_Hz, audio_settings.sample_rate_Hz); //set to false to disable
  earpieceShield.setHPFonADC(true, hardware_cutoff_Hz, audio_settings.sample_rate_Hz); //set to false to disable

  //setup BLE
	myTympan.setupBLE(); //Assumes the default Bluetooth firmware. You can override!
  
  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);
  audioSDWriter.setNumWriteChannels(4);     //can record 2 or 4 channels
  Serial.println("Setup(): SD configured for " + String(audioSDWriter.getNumWriteChannels()) + " channels.");

  //initialize the tone
  audioMixerL.mute();  //mute everything (will be set correctly by activateTone())
  audioMixerR.mute();  //mute everything (will be set correctly by activateTone())
  sineTone.setFrequency_Hz(1000.0);
  sineTone.setAmplitude_dBFS(-12.0);
  sineTone.setEnabled(false); //start as muted
  //activateTone(myState.is_tone_active);
  setToneOutputChannel(myState.tone_output_chan);  //set which channel the tone comes from

  Serial.println("Setup complete.");
  serialManager.printHelp();

} //end setup()

// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //respond to Serial commands
  if (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial

  //respond to BLE
  if (ble.available() > 0) {
    String msgFromBle; int msgLen = ble.recvBLE(&msgFromBle);
    for (int i=0; i < msgLen; i++) serialManager.respondToByte(msgFromBle[i]);
  }

  //service the SD recording
  audioSDWriter.serviceSD_withWarnings(i2s_in); //For the warnings, it asks the i2s_in class for some info
 
  // Did the user activate MTP mode?  If so, service the MTP and nothing else
  if (use_MTP) {   //service MTP (ie, the SD card appearing as a drive on your PC/Mac
     
     service_MTP();  //Find in Setup_MTP.h 
  
  } else { //do everything else!

    //service the BLE advertising state
    if (audioSDWriter.getState() != AudioSDWriter::STATE::RECORDING) {
      ble.updateAdvertising(millis(),5000); //check every 5000 msec to ensure it is advertising (if not connected)
    }

    //service the LEDs...blink slow normally, blink fast if recording
    myTympan.serviceLEDs(millis(),audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING); 
  
  }
  
  //periodically print the CPU and Memory Usage, if requested
  if (myState.flag_printCPUandMemory) myState.printCPUandMemory(millis(), 3000); //print every 3000msec  (method is built into TympanStateBase.h, which myState inherits from)
  if (myState.flag_printCPUandMemory) myState.printCPUtoGUI(millis(), 3000);     //send to App every 3000msec (method is built into TympanStateBase.h, which myState inherits from)

} //end loop()



// //////////////////////////////////// Control the audio from the SerialManager

// helper function
//float32_t overallTonePlusDacLoudness(void) { return myState.tone_dBFS + myState.output_dacGain_dB; }
float32_t overallTonePlusDacLoudness(void) { return (myState.sineTone)->getAmplitude_dBFS() + myState.output_dacGain_dB; }

//set the amplifier gain
float setDacOutputGain(float gain_dB) {
  float new_gain = min(6.0, (myState.sineTone)->getAmplitude_dBFS() + gain_dB) -(myState.sineTone)->getAmplitude_dBFS() ; //limit the gain to prevent excessive clipping (will allow some clipping...starts at -3dBFS)
  return myState.output_dacGain_dB = myTympan.volume_dB(new_gain); //yes, this sets the DAC gain, not the headphone volume
}

//increment the amplifier gain
float incrementDacOutputGain(float incr_dB) {
  return setDacOutputGain(myState.output_dacGain_dB + incr_dB);
}

//set the output channel for the tone
int setToneOutputChannel(int output_code) {
  int ret_val = output_code;
  switch (output_code) {
    case State::TONE_LEFT:
      audioMixerL.gain(0,1.0);  //unmute this side.  Tone should be connected to channel 0
      audioMixerR.gain(0,0.0);  //mute this side.  Tone should be connected to channel 0
      myState.tone_output_chan = output_code;
      break;
    case State::TONE_RIGHT:
      audioMixerL.gain(0,0.0);  //mute this side.  Tone should be connected to channel 0
      audioMixerR.gain(0,1.0);  //unmute this side.  Tone should be connected to channel 0
      myState.tone_output_chan = output_code;
      break;
    case State::TONE_BOTH:
      audioMixerL.gain(0,1.0);  //unmute this side.  Tone should be connected to channel 0
      audioMixerR.gain(0,1.0);  //unmute this side.  Tone should be connected to channel 0
      myState.tone_output_chan = output_code;
      break;
    default:
      //no change in output channel
      break; 
  }

 return myState.tone_output_chan = output_code;
}

