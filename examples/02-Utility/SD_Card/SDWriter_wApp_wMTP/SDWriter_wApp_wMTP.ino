/*
   SDWriter_wApp_wMTP
   
   Created: Chip Audette, OpenAudio, May 2019 (Updated Aug 2021, April 2024, May 2024)
   Purpose: Write audio to SD based on serial commands via USB serial or via Bluetooth (BLE).
      Access files on SD card from PC via experimental "MTP" support provided by Teensy

   For Tympan Rev D, program in Arduino IDE as a Teensy 3.6.
   For Tympan Rev E, program in Arduino IDE as a Teensy 4.1.
   For Tympan Ref F, program in Arduino IDE as a Teensy 4.1.
 
   To compile for MTP, you need to:
     * Arduino IDE 1.8.15 or later
     * Teensyduino 1.58 or later
     * MTP_Teensy library from https://github.com/KurtE/MTP_Teensy
     * In the Arduino IDE, under the "Tools" menu, choose "USB Type" and then "Serial + MTP Disk (Experimental)"

   To use MTP, you need to:
     * Connect to a PC via USB
     * Open a SerialMonitor (such as through the Arduino IDE)
     * Send the character '>' (without quotes) to the Tympan
     * After a second, it should appear in Windows as a drive
     * After using MTP to transfer files, you must cycle the power on the Tympan to get out of MTP mode.

   MTP Support is VERY EXPERIMENTAL!!  There are weird behaviors that come with the underlying
   MTP support provided by Teensy and its libraries.  

   TROUBESHOOTING:
  
   Symptom: Tympan does not appear as a drive in your operating system.
     * Be aware that you cannot record (or play) from SD and then activate MTP. If you've used the SD card
       at all, the MTP doesn't seem to work. Sorry.  The MTP only works if nothing else has used the SD card.  
     * But of course you need to use the SD card to record or play!  Why else would you want MTP? You want
	   MTP so that you can get files that you've recorded (or are going to play)!  What do you do??
	 * The easiest solution is to record and play from the SD as you desire.  Then, when you go to connect
	   to the PC, simply cycle the power on teh Tympan.  
	 * That way, it starts fresh.  The "first" time it goes to access the SD card will be for MTP.  Easy.
	 
   Symptom: The Tympan still does not appear as a drive in your operating system. 
     * This is most likely to happen immediately after you've first programmed a non-MTP Tympan
	   to use MTP.  I think that the PC (Windows, at least) gets confused as to what kind of device
	   is attached.  The way to get around this problem is to give Windows a chance to forget the device.
     * A solution seems to be to (1) turn off the Tympan, (2) unplug from USB, (3) wait 5 seconds,
	   (4) reconnect to USB, (5) power up the Tympan.
     * When the Tympan boots up and appears in the SerialMonitor again, go ahead and send the '>' character.
       It should show up as a drive on your PC!

   Symptom: the Arduino IDE cannot program the Tympan when the Tympan is running an MTP-enabled sketch like this
      one.  Similarly, you might find that the SerialMonitor doesn't seem to work with your Tympan.  These are
      similar problems.
     * Be aware that when your Tympan is running MTP-aware code, the generic serial link back to the PC is
       a little bit different, which sometimes gives the Arduino IDE trouble.  The Arduino IDE seems to be
       confused about the serial port and so reprogramming or the SerialMonitor don't work right.
     * If that happens, cycle the Tympan's power and once it has rebooted, go under the "Tools" menu,
       and click on the "Port" submenu.  You'll see (on Windows, at least) that your Tympan seems to have
       two "Teensy" entries in this menu.  Choose whichever entry wasn't already selected.  Then, try uploading
       your Arduino code again.  It should now work.
     * If it still doesn't work, cycle the Tympan power, and switch back to the original port.  It should really
       work now!
       
   MIT License.  use at your own risk.
*/


// Include all the of the needed libraries
#include <Tympan_Library.h>

//set the sample rate and block size
const float sample_rate_Hz = 44100.0f ; //24000 or 44117 or 96000 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 128;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)  Must be 128 for SD recording.
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);


// /////////// Define audio objects...they are configured later

//create audio library objects for handling the audio
Tympan                        myTympan(TympanRev::F,audio_settings);   //do TympanRev::D or TympanRev::E
AudioInputI2S_F32             i2s_in(audio_settings);   //Digital audio input from the ADC
AudioSDWriter_F32_UI          audioSDWriter(audio_settings); //this is stereo by default
AudioOutputI2S_F32            i2s_out(audio_settings);  //Digital audio output to the DAC.  Should always be last.

//Connect to outputs
AudioConnection_F32           patchcord1(i2s_in, 0, i2s_out, 0);    //Left input to left output
AudioConnection_F32           patchcord2(i2s_in, 1, i2s_out, 1);    //Right input to right output

//Connect to SD logging
AudioConnection_F32           patchcord3(i2s_in, 0, audioSDWriter, 0);   //connect Raw audio to left channel of SD writer
AudioConnection_F32           patchcord4(i2s_in, 1, audioSDWriter, 1);   //connect Raw audio to right channel of SD writer


// /////////// Create classes for controlling the system, espcially via USB Serial and via the App
#include      "SerialManager.h"
#include      "State.h"                
BLE_UI&       ble = myTympan.getBLE_UI();   //myTympan owns the ble object, but we have a reference to it here
SerialManager serialManager(&ble);     //create the serial manager for real-time control (via USB or App)
State         myState(&audio_settings, &myTympan, &serialManager); //keeping one's state is useful for the App's GUI

/* Create the MTP servicing stuff so that one can access the SD card via USB */
/* If you want this, be sure to set the USB mode via the Arduino IDE,  Tools Menu -> USB Type -> Serial + MTP (experimental) */
#include "setup_MTP.h"  //put this line sometime after the audioSDWriter has been instantiated


//set up the serial manager
void setupSerialManager(void) {
  //register all the UI elements here
  serialManager.add_UI_element(&myState);
  serialManager.add_UI_element(&ble);
  serialManager.add_UI_element(&audioSDWriter);
}


// /////////// Functions for configuring the system

float setInputGain_dB(float val_dB) {
  return myState.input_gain_dB = myTympan.setInputGain_dB(val_dB);
}

void setConfiguration(int config) {
  myState.input_source = config;
  const float default_mic_gain_dB = 10.0f;

  switch (config) {
    case State::INPUT_PCBMICS:
      //Select Input and set gain
      myState.input_source = config;
      myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones
      setInputGain_dB(default_mic_gain_dB);
      break;

    case State::INPUT_JACK_MIC:
      //Select Input and set gain
      myState.input_source = config;
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the mic jack
      myTympan.setEnableStereoExtMicBias(true);  //put the mic bias on both channels
      setInputGain_dB(default_mic_gain_dB);
      break;
      
    case State::INPUT_JACK_LINE:
      //Select Input and set gain
      myState.input_source = config;
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the line-input through holes
      setInputGain_dB(0.0);
      break;
  }
}

// ///////////////// Main setup() and loop() as required for all Arduino programs

// define the setup() function, the function that is called once when the device is booting
void setup() {
  myTympan.beginBothSerial(); delay(1000);
  Serial.println("Tympan: SDWriter_wApp_wMTP: setup():...");
  Serial.println("Sample Rate (Hz): " + String(audio_settings.sample_rate_Hz));
  Serial.println("Audio Block Size (samples): " + String(audio_settings.audio_block_samples));

  //allocate the dynamically re-allocatable audio memory
  AudioMemory_F32(100, audio_settings); 

  //activate the Tympan audio hardware
  myTympan.enable();        // activate the flow of audio
  myTympan.volume_dB(0.0);  // headphone amplifier

  //Select the input that we will use 
  setConfiguration(myState.input_source);      //use the default that is written in State.h 

  //setup BLE
  delay(500); myTympan.setupBLE(); delay(500); //Assumes the default Bluetooth firmware. You can override!
  
  //setup the serial manager
  setupSerialManager();

  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);         //the library will print any error info to this serial stream (note that myTympan is also a serial stream)
  audioSDWriter.setNumWriteChannels(2);       //this is also the built-in defaullt, but you could change it to 4 (maybe?), if you wanted 4 channels.
  Serial.println("Setup: SD configured for " + String(audioSDWriter.getNumWriteChannels()) + " channels.");
  
  //End of setup
  Serial.println("Setup: complete."); 
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

    if (audioSDWriter.getState() != AudioSDWriter::STATE::RECORDING) {
      //service the BLE advertising state as long as the SD isn't recording (it can cause SD-writing hiccups)
      ble.updateAdvertising(millis(),5000); //check every 5000 msec to ensure it is advertising (if not connected)
    }
     
    //service the LEDs...blink slow normally, blink fast if recording
    myTympan.serviceLEDs(millis(),audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING); 

  }

  //periodically print the CPU and Memory Usage
  if (myState.flag_printCPUandMemory) myState.printCPUandMemory(millis(), 3000); //print every 3000msec  (method is built into TympanStateBase.h, which myState inherits from)
  if (myState.flag_printCPUandMemory) myState.printCPUtoGUI(millis(), 3000);     //send to App every 3000msec (method is built into TympanStateBase.h, which myState inherits from)

} //end loop()


// //////////////////////////////////// Control the audio processing from the SerialManager

//here's a function to change the volume settings.   We'll also invoke it from our serialManager
float incrementInputGain(float increment_dB) {
  return setInputGain_dB(myState.input_gain_dB + increment_dB);
}
