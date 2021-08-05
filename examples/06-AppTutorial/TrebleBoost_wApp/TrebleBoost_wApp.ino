/*
*   TrebleBoost_wApp
*
*   Created: Chip Audette, OpenAudio, August 2021
*   Purpose: Process audio by applying a high-pass filter followed by gain.  Includes App interaction.
*
*   TympanRemote App: https://play.google.com/store/apps/details?id=com.creare.tympanRemote
*
*   As a tutorial on how to interact with the mobile phone App, you should first explore the previous
*   example, which was "BasicGain_wApp".
*
*   Here in "TrebleBoost_wApp", we started with the example sketch "01-Basic\TrebleBoost" and added
*   the App functionality.  The App functionality in "TrebleBoost_wApp" is the same as 
*   "BasicGain_wApp" but adds features so that you can see a more complicated example.
*
*   Like any App-enabled sketch, this example shows you:
*      * How to use Tympan code here to define a graphical interface (GUI) in the App
*      * How to transmit that GUI to the App
*      * How to respond to the commands coming back from the App
*
*   Compared to the previous example, this sketch:
*      * Adds more buttons groups ("cards") to the previous example's GUI
*      * Adds another page to the GUI
*      * Adds a button that you can illuminate to indicate state 
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
Tympan                    myTympan(TympanRev::E);     //do TympanRev::D or TympanRev::E
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

//define class for handling the GUI on the app
TympanRemoteFormatter myGUI;  //Creates the GUI-writing class for interacting with TympanRemote App

//Create BLE
BLE ble = BLE(&Serial1);

// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 10.0f;    //gain on the microphone
float digital_gain_dB = 0.0;      //this will be set by the app
float cutoff_Hz = 1000.f;             //frequencies below this will be attenuated
const float min_allowed_Hz = 100.0f;  //minimum allowed cutoff frequency (Hz)
const float max_allowed_Hz = 0.9 * (sample_rate_Hz/2.0);  //maximum allowed cutoff frequency (Hz);
bool printCPUtoGUI = false;
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
  myTympan.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  myTympan.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  //setup BLE
  while (Serial1.available()) Serial1.read(); //clear the incoming Serial1 (BT) buffer
  ble.setupBLE(myTympan.getBTFirmwareRev());  //this uses the default firmware assumption. You can override!

  //Set the cutoff frequency for the highpassfilter
  cutoff_Hz = 1000.f;  
  setHighpassFilters_Hz(cutoff_Hz);   //function defined near the bottom of this file
  printHighpassCutoff();

  Serial.println("Setup complete.");
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

  //periodically print the CPU and Memory Usage
  if (printCPUtoGUI) {
    myTympan.printCPUandMemory(millis(),3000); //print every 3000 msec
    serviceUpdateCPUtoGUI(millis(),3000);      //print every 3000 msec
  }

} //end loop();


// ///////////////// Servicing routines


//respond to the received characters (from the SerialMonitor or from the App)
void respondToByte(char c) {
  Serial.print("Received character "); Serial.println(c);
  
  switch (c) {
    case 'J': case 'j':
      printTympanRemoteLayout();
      break;
    case 'k':
      changeGain(3.0);
      printGainLevels();
      setButtonText("gainIndicator", String(digital_gain_dB,1));  //send the current value to the App.  show 1 decimal place
      break;
    case 'K':
      changeGain(-3.0);
      printGainLevels();
      setButtonText("gainIndicator", String(digital_gain_dB,1)); //send the current value to the App.  show 1 decimal place
      break;
    case 't':
      incrementHighpassFilters(powf(2.0,1.0/3.0)); //raise by 1/3rd octave
      printHighpassCutoff();
      setButtonText("cutoffHz", String(cutoff_Hz,0));   //send the current value to the App.  show 0 decimal places
      break;
    case 'T':
      incrementHighpassFilters(1.0/powf(2.0,1.0/3.0));  //lower by 1/3rd octave
      printHighpassCutoff();
      setButtonText("cutoffHz", String(cutoff_Hz,0));  //send the current value to the App.  show 0 decimal places
      break;
    case 'c':
      Serial.println("Starting CPU reporting...");
      printCPUtoGUI = true;
      setButtonState("cpuStart",printCPUtoGUI);   //show the start button as "on"
      break;
    case 'C':
      Serial.println("Stopping CPU reporting...");
      printCPUtoGUI = false;
      setButtonState("cpuStart",printCPUtoGUI);  //show the start button as "off"
      break;           
  }
}

//Test to see if enough time has passed to send up updated CPU value to the App
void serviceUpdateCPUtoGUI(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    //send the latest value to the GUI!
    setButtonText("cpuValue",String(audio_settings.processorUsage(),1)); //one decimal places
    
    lastUpdate_millis = curTime_millis;
  } // end if
} //end serviceUpdateCPUtoGUI();

// ///////////////// functions used to define the GUI on the phone


// Print the layout for the Tympan Remote app, in a JSON-ish string
// (single quotes are used here, whereas JSON spec requires double quotes.  The app converts ' to " before parsing the JSON string).
// Please don't put commas or colons in your ID strings!
void printTympanRemoteLayout(void) {
  if (myGUI.get_nPages() < 1) createTympanRemoteLayout();  //create the GUI, if it hasn't already been created
  Serial.println(myGUI.asString());
  ble.sendMessage(myGUI.asString());

  //send the values to populate the GUI with the numerical values
  setButtonText("gainIndicator", String(digital_gain_dB,1));
  setButtonText("cutoffHz", String(cutoff_Hz,0));
  setButtonText("inpGain",String(input_gain_dB,1));
  setButtonState("cpuStart",printCPUtoGUI);  //illuminate the button if we will be sending the CPU value
}

//define the GUI for the App
void createTympanRemoteLayout(void) {
  
  // Create some temporary variables
  TR_Page *page_h;  //dummy handle for a page
  TR_Card *card_h;  //dummy handle for a card

  //Add first page to GUI  (the indentation doesn't matter; it is only to help us see it better)
  page_h = myGUI.addPage("Treble Boost Demo");
      
      //Add a card under the first page
      card_h = page_h->addCard("Highpass Gain (dB)");
          //Add a "-" digital gain button with the Label("-"); Command("K"); Internal ID ("minusButton"); and width (4)
          card_h->addButton("-","K","",           4);  //displayed string, command, button ID, button width (out of 12)

          //Add an indicator that's a button with no command:  Label (value of the digital gain); Command (""); Internal ID ("gain indicator"); width (4).
          card_h->addButton("","","gainIndicator",4);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
  
          //Add a "+" digital gain button with the Label("+"); Command("K"); Internal ID ("minusButton"); and width (4)
          card_h->addButton("+","k","",           4);   //displayed string, command, button ID, button width (out of 12)

      //Add another card to this page
      card_h = page_h->addCard("Highpass Cutoff (Hz)");
          card_h->addButton("-","T","",        4);  //displayed string, command, button ID, button width (out of 12)
          card_h->addButton("", "", "cutoffHz",4);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
          card_h->addButton("+","t","",        4);  //displayed string, command, button ID, button width (out of 12)


  //Add a second page to the GUI
  page_h = myGUI.addPage("Globals");

    //Add an example card that just displays a value...no interactive elements
    card_h = page_h->addCard(String("Analog Input Gain (dB)"));
      card_h->addButton("",      "", "inpGain",   12); //label, command, id, width (out of 12)...THIS ISFULL WIDTH!

    //Add an example card where one of the buttons will indicate "on" or "off"
    card_h = page_h->addCard(String("CPU Usage (%)"));
      card_h->addButton("Start", "c", "cpuStart", 4);  //label, command, id, width...we'll light this one up if we're showing CPU usage
      card_h->addButton(""     , "",  "cpuValue", 4);  //label, command, id, width...this one will display the actual CPU value
      card_h->addButton("Stop",  "C", "",         4);  //label, command, id, width...this one will turn off the CPU display
        
  //add some pre-defined pages to the GUI (pages that are built-into the App)
  myGUI.addPredefinedPage("serialMonitor");
  //myGUI.addPredefinedPage("serialPlotter");
}



// ///////////////// functions used to respond to the commands

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

//change the highpass cutoff frequency
void incrementHighpassFilters(float scaleFactor) { setHighpassFilters_Hz(cutoff_Hz*scaleFactor); }
float setHighpassFilters_Hz(float freq_Hz) {
  cutoff_Hz = min(max(freq_Hz,min_allowed_Hz),max_allowed_Hz); //set the new value as the new state
  hp_filt1.setHighpass(0, cutoff_Hz); //biquad IIR filter.  left channel
  hp_filt2.setHighpass(0, cutoff_Hz); //biquad IIR filter.  right channel
  return cutoff_Hz;
}

void printHighpassCutoff(void) {
  Serial.println("Highpass filter cutoff at " + String(cutoff_Hz,0) + " Hz");
}


// ///////////////// functions used to communicate back to the App

void setButtonText(String btnId, String text) {
  String str = "TEXT=BTN:" + btnId + ":"+text;
  Serial.println(str);
  ble.sendMessage(str);
}

void setButtonState(String btnId, bool newState) {
  String str = String("STATE=BTN:") + btnId;
  if (newState) {
    str = str + String(":1");
  } else {
    str = str + String(":0");
  }
  ble.sendMessage(str);
}
