/*
*   BasicGain_wApp
*
*   Created: Chip Audette, June 2020 (updated June 2021 for BLE)
*   Purpose: Process audio using Tympan by applying gain.
*      Also, illustrate how to change Tympan setting via the TympanRemote App.
*
*   TympanRemote App: https://play.google.com/store/apps/details?id=com.creare.tympanRemote
*
*   As a tutorial on how to interact with the mobile phone App, this example should be your
*   first step.  In this example, we started with the example sketch "01-Basic\BasicGain" and
*   added functionality to interact with the App.  
*
*   Because BasicGain was such a basic example (it only applied gain, which would change
*   the overal loudness from the Tympan), this "BasicGain_wApp" is similarly basic.  It
*   shows you how to change the loudness of the Tympan from the App.  
*
*   Like any App-enabled sketch, this example shows you:
*      * How to use Tympan code here to define a graphical interface (GUI) in the App
*      * How to transmit that GUI to the App
*      * How to respond to the commands coming back from the App
*
*   The core of each of these three elements can be see in these functions:
*      * Define the GUI: see "createTympanRemoteLayout()"
*      * Transmit the GUI: see "printTympanRemoteLayout"
*      * Respond to commands: see "respondsToByte()" and the functions it calls
*
*   MIT License.  use at your own risk.
*/

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


// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 10.0f; //gain on the microphone
float digital_gain_dB = 0.0;      //this will be set by the app
void setup() {

  //begin the serial comms (for debugging)
  myTympan.beginBothSerial();delay(1000);
  Serial.println("BasicGain_wApp: starting setup()...");

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
  
	//finish
	Serial.println("Setup complete.");
	printHelp();
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {
  
  //look for in-coming serial messages (via USB or via Bluetooth)
  if (Serial.available()) respondToByte((char)Serial.read());   //USB Serial

  //respond to BLE
  if (ble.available() > 0) {
    String msgFromBle; int msgLen = ble.recvBLE(&msgFromBle);
    for (int i=0; i < msgLen; i++) respondToByte(msgFromBle[i]);
  }

  //service the BLE advertising state
   ble.updateAdvertising(millis(),5000); //check every 5000 msec to ensure it is advertising (if not connected)

} //end loop();


// ///////////////// Servicing routines
void printHelp(void) {  //not required for the App. But, is helpful for debugging via USB Serial
	Serial.println("BasicGain_wApp: Available Commands:");
  Serial.println(" h  : Print this help");
  Serial.println(" k/K: Incr/Decrease gain");
	Serial.println(" J  : Print Tympan GUI for Tympan Remote App");
	Serial.println();
}

//respond to serial commands
void respondToByte(char c) {
  if ((c != ' ') && (c != '\n') && (c != '\r')) { Serial.print("Received character "); Serial.println(c);}
  switch (c) {
    case 'J': case 'j':           //The TympanRemote app sends a 'J' to the Tympan when it connects
      printTympanRemoteLayout();  //in resonse, the Tympan sends the definition of the GUI that we'd like
      break;
    case 'h':
			printHelp();  //not required for the App. But, is helpful for debugging via USB Serial
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
