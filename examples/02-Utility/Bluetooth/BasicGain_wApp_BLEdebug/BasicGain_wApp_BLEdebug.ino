/*
*   BasicGain_wApp_BLEdebug
*
*   Created: Chip Audette, June 2020 (updated Aug 2021 for BLE debugging)
*   Purpose: Process audio using Tympan by applying gain.
*      Also, illustrate how to change Tympan setting via the TympanRemote App.
*
*   TympanRemote App: https://play.google.com/store/apps/details?id=com.creare.tympanRemote
*
*   This sketch follows the AppTutorial example "BasicGain_wApp" but adds a bunch of BLE
*   debugging messages.  It also adds commands to the Serial Monitor menu to allow you 
*   to manually enable/disable BLE advertising and BT_Class discoverability.
*
*   MIT License.  use at your own risk.
*/

#define PRINT_BLE_DEBUG true   //set to true to show lots of BLE debugging info in the Serial Monitor

//here are the libraries that we need
#include <Tympan_Library.h>  //include the Tympan Library

//set the sample rate and block size
const float sample_rate_Hz = 44100.0f ; //24000 or 44100 (or 44117, or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 32;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
Tympan                    myTympan(TympanRev::E, audio_settings);     //do TympanRev::D or TympanRev::E
AudioInputI2S_F32         i2s_in;                     //Digital audio in *from* the Teensy Audio Board ADC.
AudioEffectGain_F32       gain1;                      //Applies digital gain to audio data.  Left.
AudioEffectGain_F32       gain2;                      //Applies digital gain to audio data.  Right.
AudioOutputI2S_F32        i2s_out;                    //Digital audio out *to* the Teensy Audio Board DAC.

//Make all of the audio connections
AudioConnection_F32       patchCord1(i2s_in, 0, gain1, 0);    //connect the Left input
AudioConnection_F32       patchCord2(i2s_in, 1, gain2, 0);    //connect the Right input
AudioConnection_F32       patchCord11(gain1, 0, i2s_out, 0);  //connect the Left gain to the Left output
AudioConnection_F32       patchCord12(gain2, 0, i2s_out, 1);  //connect the Right gain to the Right output

//define class for handling the GUI on the app
TympanRemoteFormatter myGUI;  //Creates the GUI-writing class for interacting with TympanRemote App

//Create BLE
BLE ble(&myTympan);
bool flag_autoAdvertise = true;

// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 10.0f; //gain on the microphone
float digital_gain_dB = 0.0;      //this will be set by the app
void setup() {

  //begin the serial comms (for debugging)
  myTympan.beginBothSerial();delay(1000);
  Serial.println("BasicGain_wApp_BLEdebug: starting setup()...");

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(10); 

  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate the Tympan's audio module

  //Choose the desired input
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  myTympan.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  myTympan.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps
  gain1.setGain_dB(digital_gain_dB);       //set the digital gain of the Left-channel
  gain2.setGain_dB(digital_gain_dB);       //set the digital gainof the Right-channel gain processor  

  //setup BLE
  while (Serial1.available()) Serial1.read(); //clear the incoming Serial1 (BT) buffer
  ble.setupBLE(myTympan.getBTFirmwareRev());  //this uses the default firmware assumption. You can override!
  ble.version(true); //the "true" prints the results to the Serial Monitor

  //send help menu for serial commands via the Serial Monitor
  printHelp();


  Serial.println("Setup complete.");
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {
  
  //look for in-coming serial messages (via USB or via Bluetooth)
  if (Serial.available()) respondToByte((char)Serial.read());   //USB Serial

  //respond to BLE
  while (ble.available()) {
    String msgFromBle; int msgLen = ble.recvBLE(&msgFromBle,PRINT_BLE_DEBUG);
    for (int i=0; i < msgLen; i++) respondToByte(msgFromBle[i]);
  }

  //service the BLE advertising state
  if (flag_autoAdvertise) {
    ble.updateAdvertising(millis(),5000,PRINT_BLE_DEBUG); //check every 5000 msec to ensure it is advertising (if not connected)
  } else {
    if (PRINT_BLE_DEBUG) printBleStatus(millis(), 5000);
  }

} //end loop();

// //////////////// Serviding routines

void printBleStatus(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
static unsigned long lastUpdate_millis = 0;
  static float prev_val = -1.0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    
    ble.status(true);    //the "true" prints the results to the Serial Monitor
    
    lastUpdate_millis = curTime_millis;
  }
}


// ///////////////// Serial / App related routines routines

//print menu
void printHelp(void) {
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println(" h: Print this help");
  Serial.println(" k/K: Increase/Decrease gain");
  Serial.println(" a/A: Activate/Deactivate auto-restart of BLE advertising");
  Serial.println(" b/B: Activate/Deactivate BLE advertising");
  Serial.println(" c/C: Activate/Deactivate BT Classic discoverable connectable");
  Serial.println(" 0:   V7: Close any Bluetooth Link 10, 20, 30");
  Serial.println(" 1:   V7: Close any Bluetooth Link 11, 21, 31");
  Serial.println(" 2:   V7: Close any Bluetooth Link 12, 22, 32");
  Serial.println(" 3:   V7: Close any Bluetooth Link 13, 23, 33");
  Serial.println(" 4:   V7: Close any Bluetooth Link 14, 24, 34");
  Serial.println(" s:   print Bluetooth status");
  Serial.println(" v:   print Bluetooth Firmware version info");
}

//respond to serial commands
void respondToByte(char c) {

  if ((c != '\n') && (c != '\r')) {
    Serial.print("respondToByte: received character from Serial or Bluetooth: "); Serial.println(c);
  }
  
  switch (c) {
    case 'h':
      printHelp();
      break;
    case 'J': case 'j':           //The TympanRemote app sends a 'J' to the Tympan when it connects
      printTympanRemoteLayout();  //in resonse, the Tympan sends the definition of the GUI that we'd like
      break;
    case 'k':
      changeGain(3.0);
      printGainLevels();
      setButtonText("gainIndicator", String(digital_gain_dB));
      break;
    case 'K':
      changeGain(-3.0);
      printGainLevels();
      setButtonText("gainIndicator", String(digital_gain_dB));
      break;
    case 'a':
      flag_autoAdvertise = true;
      Serial.println("respondToByte: activating auto-restart of BLE advertising..." + String(flag_autoAdvertise));   
      break;
    case 'A':
      flag_autoAdvertise = false;
      Serial.println("respondToByte: de-activating auto-restart of BLE advertising..." + String(flag_autoAdvertise));
      break;       
    case 'b':
      Serial.println("respondToByte: activating BLE advertising...");
      ble.advertise(true);
      break;
    case 'B':
      Serial.println("respondToByte: de-activating BLE advertising...");
      ble.advertise(false);
      break;
    case 'c':
      Serial.println("respondToByte: activating BT Classic discoverable...");
      if (ble.get_BC127_firmware_ver() >= 7)  {
        ble.discoverableConnectableV7(true);
      } else {
        ble.discoverable(true);
      }
      break;
    case 'C':
      Serial.println("respondToByte: de-activating BT Classic discoverable...");
      if (ble.get_BC127_firmware_ver() >= 7)  {
        ble.discoverableConnectableV7(false);
      } else {
        ble.discoverable(false);
      }
      break; 
    case '0':
      Serial.println("respondToByte: closing BLE Link 10, 20, 30...");
      ble.close(10);ble.close(20);ble.close(30);
      break;       
    case '1':
      Serial.println("respondToByte: closing BLE Link 11, 21, 31...");
      ble.close(11);ble.close(21);ble.close(31);
      break;       
    case '2':
      Serial.println("respondToByte: closing BLE Link 12, 22, 32...");
      ble.close(12);ble.close(22);ble.close(32);
      break;        
    case '3':
      Serial.println("respondToByte: closing BLE Link 13, 23, 33...");
      ble.close(13);ble.close(23);ble.close(33);
      break;        
    case '4':
      Serial.println("respondToByte: closing BLE Link 14, 24, 34...");
      ble.close(14);ble.close(24);ble.close(34);
      break;        
    case 's':
      Serial.println("respondToByte: printing Bluetooth status...");
      ble.status(true);
      break;     
    case 'v':
      Serial.println("respondToByte: printing Bluetooth firmware version info...");
      ble.version(true);
      break;     
  }
}

// Print the layout for the Tympan Remote app, in a JSON-ish string
// (single quotes are used here, whereas JSON spec requires double quotes.  The app converts ' to " before parsing the JSON string).
// Please don't put commas or colons in your ID strings!
void printTympanRemoteLayout(void) {
  if (myGUI.get_nPages() < 1) createTympanRemoteLayout();  //create the GUI, if it hasn't already been created
  Serial.println(myGUI.asString());
  ble.sendMessage(myGUI.asString());
  setButtonText("gainIndicator", String(digital_gain_dB));
}

//define the GUI for the App
void createTympanRemoteLayout(void) {
  
  // Create some temporary variables
  TR_Page *page_h;  //dummy handle for a page
  TR_Card *card_h;  //dummy handle for a card

  //Add first page to GUI
  page_h = myGUI.addPage("Basic Gain Demo");
      //Add a card under the first page
      card_h = page_h->addCard("Change Loudness");
          //Add a "-" digital gain button with the Label("-"); Command("K"); Internal ID ("minusButton"); and width (4)
          card_h->addButton("-","K","minusButton",4);  //displayed string, command, button ID, button width (out of 12)

          //Add an indicator that's a button with no command:  Label (value of the digital gain); Command (""); Internal ID ("gain indicator"); width (4).
          card_h->addButton("","","gainIndicator",4);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
  
          //Add a "+" digital gain button with the Label("+"); Command("K"); Internal ID ("minusButton"); and width (4)
          card_h->addButton("+","k","plusButton",4);   //displayed string, command, button ID, button width (out of 12)
        
  //add some pre-defined pages to the GUI
  myGUI.addPredefinedPage("serialMonitor");
  //myGUI.addPredefinedPage("serialPlotter");
}


//change the gain from the App
void changeGain(float change_in_gain_dB) {
  digital_gain_dB = digital_gain_dB + change_in_gain_dB;
  gain1.setGain_dB(digital_gain_dB);  //set the gain of the Left-channel gain processor
  gain2.setGain_dB(digital_gain_dB);  //set the gain of the Right-channel gain processor
}


//Print gain levels 
void printGainLevels(void) {
  Serial.print("Analog Input Gain (dB) = "); 
  Serial.println(input_gain_dB); //print text to Serial port for debugging
  Serial.print("Digital Gain (dB) = "); 
  Serial.println(digital_gain_dB); //print text to Serial port for debugging
}

void setButtonText(String btnId, String text) {
  String str = "TEXT=BTN:" + btnId + ":"+text;
  Serial.println(str);
  ble.sendMessage(str);
}
