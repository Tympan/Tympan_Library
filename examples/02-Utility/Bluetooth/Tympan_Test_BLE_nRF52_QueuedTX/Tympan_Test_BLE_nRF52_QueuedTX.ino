/*
*   Tympan_Test_BLE_nRF52_QueuedTX
*
*   Created: Chip Audette, OpenAudio, April 2024
*            Updated May 2024 for updated BLE_nRF52 in Tympan_Library
*   Purpose: Test the nRF52840 on the Tympan RevF.  Includes App interaction.
*            Uses the BLE module's queue to enable the Tympan to dump more many
*            messages onto the module without the Tympan having to wait for the
*            transmissions to be complete (requires BLE Firmware 0.5.0 or newer). 
*
*   TympanRemote App: https://play.google.com/store/apps/details?id=com.creare.tympanRemote
*   
*   For Tympan RevF, which requires you to set the Arduino IDE to compile for Teensy 4.1
*   
*   See the comments at the top of SerialManager.h and State.h for more description.
*
*   MIT License.  use at your own risk.
*/

//here are the libraries that we need
#include <Tympan_Library.h>  //include the Tympan Library
#include "SerialManager.h"
#include "State.h"

//set the sample rate and block size
const float sample_rate_Hz = 44100.0f ; //24000 or 44100 (or 44117, or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 128;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
Tympan                    myTympan(TympanRev::F,audio_settings);          //only TympanRev::F has the nRF52480
AudioInputI2S_F32         i2s_in(audio_settings);                         //Digital audio in *from* the Teensy Audio Board ADC.
AudioEffectGain_F32       gain1(audio_settings), gain2(audio_settings);   //Applies digital gain to audio data.  Left and right.
AudioOutputI2S_F32        i2s_out(audio_settings);                        //Digital audio out *to* the Teensy Audio Board DAC.

//Make all of the audio connections
AudioConnection_F32       patchCord1(i2s_in, 0, gain1, 0);   //connect the Left input
AudioConnection_F32       patchCord2(i2s_in, 1, gain2, 0);   //connect the Right input
AudioConnection_F32       patchCord5(gain1, 0, i2s_out, 0);     //connect the Left gain to the Left output
AudioConnection_F32       patchCord6(gain2, 0, i2s_out, 1);     //connect the Right gain to the Right output

// Create classes for controlling the system
#include      "SerialManager.h"
#include      "State.h"                            
BLE_nRF52&    ble = myTympan.getBLE_nRF52(); //get BLE object for the nRF52840 (RevF only!)
SerialManager serialManager;                 //create the serial manager for real-time control (via USB or App)
State         myState; //keeping one's state is useful for the App's GUI

bool flag_useBleQueue = true;

// function to check the bluetooth version
bool isBleFirmwareRevisionOK(bool flag_printInfoToSerial) { //returns true if OK
  bool flag_firmwareVersionOK = false;

  //get the BLE firmware revision from the BLE module
  String version_string_reply;
  ble.version(&version_string_reply);
  if (flag_printInfoToSerial) Serial.println("isBleFirmwareRevisionOK: BLE Firmware Version is: " + version_string_reply);

  //find first index that is a digit
  int ind = 0;
  int str_len =  (int)version_string_reply.length(); //length() returns unsigned int, which is annoying for doing our subtraction operation later
  while (ind < str_len) {
    char c = version_string_reply[ind];
    if ((c >= '0') && (c <= '9')) break;  //find first digit
    ind++;
  }
  //Serial.println("isBleFirmwareRevisionOK: evaluating from index " + String(ind) + " which is " + version_string_reply.substring(ind));
  if ((str_len - ind) < 5) return flag_firmwareVersionOK;  //must be of format x.x.x, so must be at least 5 digits long

  //check against the requirement for 0.5.0 or higher
  if ((version_string_reply[ind] > '0') && (version_string_reply[ind] <= '9')) { flag_firmwareVersionOK = true; return flag_firmwareVersionOK; }     //any 1.x.x or higher is OK
  if ((version_string_reply[ind+2] >= '5') && (version_string_reply[ind+2] <= '9')) { flag_firmwareVersionOK = true; return flag_firmwareVersionOK; }  //any 0.5.x or higher is OK
  //if (version_string_reply[ind+4] > '0') { flag_firmwareVersionOK = true; return flag_firmwareVersionOK; } //no check here needed
  
  //print warning about version being too old
  if ((flag_firmwareVersionOK==false) && (flag_printInfoToSerial)) {
    Serial.println("isBleFirmwareRevisionOK: *** WARNING ***: Your Tympan's BLE module needs newer firmware!  At least 0.5.0 is needed!");
  }

  return flag_firmwareVersionOK;
}

// void printAnyCommsFromBleSerial(void) {
//   if (Serial7.available()) {
//     Serial.print("From Serial7: ");
//     while (Serial7.available() > 0) {
//       Serial.print(Serial7.read());
//     }
//   }
// }

// define the setup() function, the function that is called once when the device is booting
void setup() {
  //begin the serial comms (for debugging)
  //Serial.begin(115200);  //USB Serial.  This begin() isn't really needed on Teensy. 
  myTympan.beginBluetoothSerial(); //should use the correct Serial port and the correct baud rate
  //Serial7.attachRTS(1); Serial7.attachCTS(0); //pins per Tympan RevF schematic
  delay(500);  //give the BLE module time to wake up?
  Serial.println("Tympan_Test_BLE_nRF52: Starting setup()...");

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
  myTympan.setupBLE();
  
  //get BLE version (to make sure it's new enough)
  bool printInfoToSerial = true;
  flag_useBleQueue = isBleFirmwareRevisionOK(printInfoToSerial);

  Serial.println("Setup complete.");
  serialManager.printHelp();
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //look for in-coming serial messages via USB
  if (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial

  //respond to in coming serial messages via BLE
  if (ble.available() > 0) {
    String msgFromBle; ble.recvBLE(&msgFromBle);    //get BLE messages (removing non-payload messages)
    if (1) {
      //fully functioning
      for (unsigned int i=0; i < msgFromBle.length(); i++) serialManager.respondToByte(msgFromBle[i]); //interpet each character of the message
    } else {
      //protective
      if (0) {
        //simply convey the raw characters to the screen
        Serial.print(msgFromBle);
      } else {
        //or, print one character per line in a potentially more-legible way

        if ((msgFromBle.length() == 1) && ((msgFromBle[0] == '\r') || (msgFromBle[0] == '\n'))) {
          // skip printing this message
        } else {
          //print the message
          bool flag_anyPrintable = false;
          for (unsigned int i=0; i<msgFromBle.length(); i++) {
            char c = msgFromBle[i];
            if ((c >= '!') && (c <= '~')) flag_anyPrintable = true; //ascii table says that these bounds should catch all printable        
          }
          if (flag_anyPrintable) {
            Serial.println("loop(): BLE: " + msgFromBle);
          } else {
            Serial.print("loop(): BLE (all are white): HEX values = ");
            for (unsigned int i=0; i<msgFromBle.length(); i++) {
              Serial.print(" 0x"); 
              if (((int)msgFromBle[i]) < 16) Serial.print(0);
              Serial.print(msgFromBle[i],HEX);
            }
            Serial.println();
          }
          //if it is a 'J', act on it
          if ((msgFromBle.length() == 1) && ((msgFromBle[0] == 'J') || (msgFromBle[0] == 'j'))) serialManager.respondToByte(msgFromBle[0]);
        }
      }
    }
  }

  //service the BLE advertising state...not needed for RevF as it automatically re-starts advertising
  //ble.updateAdvertising(millis(),5000); //if not connected, ensure it's advertising (this line only needed for Tympan RevD and RevE)


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

