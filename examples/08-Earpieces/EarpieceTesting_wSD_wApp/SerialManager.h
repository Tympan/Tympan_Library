
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>
#include "State.h"


//Extern variables from the main *.ino file
extern Tympan myTympan;                    //created in the main *.ino file
extern State myState;                      //created in the main *.ino file
extern EarpieceMixer_F32_UI earpieceMixer; //created in the main *.ino file
extern AudioSDWriter_F32_UI audioSDWriter;


//Extern Functions
extern void setInputConfiguration(int);
extern float incrementInputGain(float);
extern int setToneOutputChannel(int);
extern float incrementDacOutputGain(float incr_dB);
extern float overallTonePlusDacLoudness(void);

//externals for MTP
extern void start_MTP(void);
//extern void stop_MTP(void);

class SerialManager : public SerialManagerBase  {  // see Tympan_Library for SerialManagerBase for more functions!
  public:
    SerialManager(BLE *_ble) : SerialManagerBase(_ble) {};

    void printHelp(void);
    void createTympanRemoteLayout(void); 
    void printTympanRemoteLayout(void); 
    bool processCharacter(char c);  //this is called automatically by SerialManagerBase.respondToByte(char c)

    //method for updating the GUI on the App
    void setFullGUIState(bool activeButtonsOnly = false);
    void updateGUI_outChan(bool activeButtonsOnly = false);

    int bigLoudnessIncrement_dB = 6.0;
    int smallLoudnessIncrement_dB = 1.0;

  private:
    TympanRemoteFormatter myGUI;  //Creates the GUI-writing class for interacting with TympanRemote App    
};


void SerialManager::printHelp(void) {  
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println(" General: No Prefix");
  Serial.println("   (,),0: TONE: Output the tone from left/right/both (currently " + myState.getToneOutputChanString() + ")");
  Serial.println("   b/B: OUTPUT : Incr/decrease DAC output gain by " + String(bigLoudnessIncrement_dB) + " dB (cur = " + String(myState.output_dacGain_dB,1) + " dB)");
  Serial.println("   g/G: OUTPUT : Incr/decrease DAC output gain by " + String(smallLoudnessIncrement_dB) + " dB (cur = " + String(myState.output_dacGain_dB,1) + " dB)");
  Serial.println("   r/s: SD : Start/Stop recording");
  #if defined(USE_MTPDISK) || defined(USB_MTPDISK_SERIAL)  //detect whether "MTP Disk" or "Serial + MTP Disk" were selected in the Arduino IDEA
    Serial.println("   >  : SD   : Start MTP mode to read SD from PC");
  #endif
  
  //Add in the printHelp() that is built-into the other UI-enabled system components.
  //The function call below loops through all of the UI-enabled classes that were
  //attached to the serialManager in the setupSerialManager() function used back
  //in the main *.ino file.
  SerialManagerBase::printHelp();  ////in here, it automatically loops over the different UI elements issuing printHelp()
  
  //Serial.println();
  //Serial.println("Overall Tone+DAC Loudness is Currently = " + String(overallTonePlusDacLoudness(),1) + " dBFS");
}


//switch yard to determine the desired action
bool SerialManager::processCharacter(char c) { //this is called by SerialManagerBase.respondToByte(char c)
  bool ret_val = true; //assume at first that we will find a match
  switch (c) {
    case 'h': 
      printHelp(); 
      break;
    case 'J': case 'j':           //The TympanRemote app sends a 'J' to the Tympan when it connects
      printTympanRemoteLayout();  //in resonse, the Tympan sends the definition of the GUI that we'd like
      break; 
    case '(':
      setToneOutputChannel(State::TONE_LEFT);
      Serial.println("Set tone output to " + myState.getToneOutputChanString());
      updateGUI_outChan();
      break;
    case ')':
      setToneOutputChannel(State::TONE_RIGHT);
      Serial.println("Set tone output to " + myState.getToneOutputChanString());
      updateGUI_outChan();
      break;
    case '0':
      setToneOutputChannel(State::TONE_BOTH);
      Serial.println("Set tone output to " + myState.getToneOutputChanString());
      updateGUI_outChan();
      break;
    case 'b':
      incrementDacOutputGain(bigLoudnessIncrement_dB);  // bigger increase
      Serial.println("Increased DAC gain to " + String(myState.output_dacGain_dB,1) + " dB so that Tone+DAC yields = " + String(overallTonePlusDacLoudness(),1) + " dBFS");
      break;
    case 'B':
      incrementDacOutputGain(-bigLoudnessIncrement_dB);  // bigger decrease
      Serial.println("Decreased DAC gain to " + String(myState.output_dacGain_dB,1) + " dB so that Tone+DAC yields = " + String(overallTonePlusDacLoudness(),1) + " dBFS");
      break;
    case 'g':
      incrementDacOutputGain(smallLoudnessIncrement_dB);  // smaller increase
      Serial.println("Increased DAC gain to " + String(myState.output_dacGain_dB,1) + " dB so that Tone+DAC yields = " + String(overallTonePlusDacLoudness(),1) + " dBFS");
      break;
    case 'G':
      incrementDacOutputGain(-smallLoudnessIncrement_dB);  // smaller decrease
      Serial.println("Decreased DAC gain to " + String(myState.output_dacGain_dB,1) + " dB so that Tone+DAC yields = " + String(overallTonePlusDacLoudness(),1) + " dBFS");
      break;
    case 'r':
      audioSDWriter.startRecording();
      Serial.println("Starting recording to SD file " + audioSDWriter.getCurrentFilename());
      break;
    case 's':
      Serial.println("Stopping recording to SD file " + audioSDWriter.getCurrentFilename());
      audioSDWriter.stopRecording();
      break;
    case '>':
      Serial.println("Starting MTP service to access SD card (everything else will stop)");
      start_MTP();
      break;
    default:
      ret_val = SerialManagerBase::processCharacter(c);  //in here, it automatically loops over the different UI elements
      break;
  }
  return ret_val;
}



// //////////////////////////////////  Methods for defining the GUI and transmitting it to the App

//define the GUI for the App
void SerialManager::createTympanRemoteLayout(void) {
  
  // Create some temporary variables
  TR_Page *page_h;  //dummy handle for a page
  TR_Card *card_h;  //dummy handle for a card

  //add page for tone controls
  page_h = sineTone.addPage_default(&myGUI);
  
  //add card to this page to control output as left or right
  card_h = page_h->addCard("Output Earpiece");
  card_h->addButton("Left", "(",  "toneL", 4);  //label, command, id, width
	card_h->addButton("Both", "0",  "toneB", 4);  //label, command, id, width
	card_h->addButton("Right", ")", "toneR", 4);  //label, command, id, width

  //add page for selecting audio input
  //page_h = myGUI.addPage("Earpiece Tester");
  //    //select inputs
  //    earpieceMixer.addCard_audioSource(page_h); //use its predefined group of buttons for input audio source

  //add page for more control of earpieces   
  //page_h = earpieceMixer.addPage_digitalEarpieces(&myGUI); //use its predefined page for controlling the digital earpieces

  //add page for SD recording
  page_h = audioSDWriter.addPage_default(&myGUI);

               
  //add some pre-defined pages to the GUI (pages that are built-into the App)
  //myGUI.addPredefinedPage("serialPlotter");  //if we send data in the right format, the App will plot the signal levels in real-time!
  myGUI.addPredefinedPage("serialMonitor");
}

// Print the layout for the Tympan Remote app, in a JSON-ish string
void SerialManager::printTympanRemoteLayout(void) {
    if (myGUI.get_nPages() < 1) createTympanRemoteLayout();  //create the GUI, if it hasn't already been created
    String s = myGUI.asString();
    Serial.println(s);
    ble->sendMessage(s); //ble is held by SerialManagerBase
    setFullGUIState();
}

// //////////////////////////////////  Methods for updating the display on the GUI

void SerialManager::setFullGUIState(bool activeButtonsOnly) {  //the "activeButtonsOnly" isn't used here, so don't worry about it
  
  //updateGUI_toneParams(activeButtonsOnly);
  updateGUI_outChan();

  //Then, let's have the system automatically update all of the individual UI elements that we attached
  //to the serialManager via the setupSerialManager() function used back in the main *.ino file.
  SerialManagerBase::setFullGUIState(activeButtonsOnly); //in here, it automatically loops over the different UI elements

}

void SerialManager::updateGUI_outChan(bool activeButtonsOnly) {
  switch (myState.tone_output_chan) {
    case (State::TONE_LEFT):
      setButtonState("toneL",true);
      if (!activeButtonsOnly) {
        setButtonState("toneR",false);
        setButtonState("toneB",false);          
      }
      break;
    case (State::TONE_RIGHT):
      setButtonState("toneR",true);
      if (!activeButtonsOnly) {
        setButtonState("toneL",false);
        setButtonState("toneB",false);          
      }
      break;
    case (State::TONE_BOTH):
      setButtonState("toneB",true);
      if (!activeButtonsOnly) {
        setButtonState("toneL",false);
        setButtonState("toneR",false);          
      }
      break;
      
  }
}


#endif
