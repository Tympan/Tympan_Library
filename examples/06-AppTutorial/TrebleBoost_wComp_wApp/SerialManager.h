
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>
#include "State.h"

//classes from the main sketch that might be used here
extern Tympan myTympan;                  //created in the main *.ino file
extern State myState;                    //created in the main *.ino file
extern AudioSettings_F32 audio_settings; //created in the main *.ino file  

//functions in the main sketch that I want to call from here
extern void incrementHighpassFilters(float);
extern void printHighpassCutoff(void);

//
// The purpose of this class is to be a central place to handle all of the interactions
// to and from the SerialMonitor or TympanRemote App.  Therefore, this class does things like:
//
//    * Define what buttons and whatnot are in the TympanRemote App GUI
//    * Define what commands that your system will respond to, whether from the SerialMonitor or from the GUI
//    * Send updates to the GUI based on the changing state of the system
//
// You could achieve all of these goals without creating a dedicated class.  But, it 
// is good practice to try to encapsulate some of these functions so that, when the
// rest of your code calls functions related to the serial communciation (USB or App),
// you have a better idea of where to look for the code and what other code it relates to.
//

//now, define the Serial Manager class
class SerialManager : public SerialManagerBase  {  // see Tympan_Library for SerialManagerBase for more functions!
  public:
    SerialManager(BLE *_ble) : SerialManagerBase(_ble) {};
      
    void printHelp(void);
    void createTympanRemoteLayout(void); 
    void printTympanRemoteLayout(void); 
    bool processCharacter(char c);  //this is called automatically by SerialManagerBase.respondToByte(char c)

    //method for updating the GUI on the App
    void setFullGUIState(bool activeButtonsOnly = false);
    void updateGainDisplay(void);
    void updateFilterDisplay(void);
    void updateCpuDisplayOnOff(void);
    void updateCpuDisplayUsage(void);
    void sendDataToAppSerialPlotter(const String data1, const String data2);

    //factors by which to raise or lower the parameters when receiving commands from TympanRemote App
    float freqIncrement = powf(2.0,1.0/3.0);  //raise or lower by 1/3rd octave
  private:

    TympanRemoteFormatter myGUI;  //Creates the GUI-writing class for interacting with TympanRemote App
   
};

void SerialManager::printHelp(void) {  
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println(" h: Print this help");
  Serial.println(" t/T: Incr/Decrease Cutoff of Highpass Filter");
  Serial.println(" ],}: Start/stop sending data to the App SerialPlotter");

  //Add in the printHelp() that is built-into the other UI-enabled system components.
  //The function call below loops through all of the UI-enabled classes that were
  //attached to the serialManager in the setupSerialManager() function used back
  //in the main *.ino file.
  SerialManagerBase::printHelp();  ////in here, it automatically loops over the different UI elements issuing printHelp()
  
  Serial.println();
}

//switch yard to determine the desired action...this method is much shorter now that we're using a lot
//of logic that has already been built-into the UI-enabled classes.
bool SerialManager::processCharacter(char c) {  //this is called by SerialManagerBase.respondToByte(char c)
  bool ret_val = true;

  switch (c) {
    case 'h':
      printHelp(); 
      break;
    case 'J': case 'j':           //The TympanRemote app sends a 'J' to the Tympan when it connects
      printTympanRemoteLayout();  //in resonse, the Tympan sends the definition of the GUI that we'd like
      break;
    case 't':
      incrementHighpassFilters(freqIncrement);     //raise
      printHighpassCutoff();  //print to USB Serial (ie, to the SerialMonitor)
      updateFilterDisplay();  //update the App
      break;
    case 'T':
      incrementHighpassFilters(1.0/freqIncrement); //lower
      printHighpassCutoff();  //print to USB Serial (ie, to the SerialMonitor)
      updateFilterDisplay();  //update the App
      break;
    case ']':
      //this is the secret code from the App to START sending data for plotting
      Serial.println("SerialManager: starting data flow for App SerialPlotter...");
      myState.flag_printSignalLevels = true;
      break;
    case '}':
      //this is the secret code from the App to STOP sending data for plotting
      Serial.println("SerialManager: stopping data flow for App SerialPlotter...");
      myState.flag_printSignalLevels = false;
      break;
    default:
      //if the command didn't match any of the commands, it loops through the processCharacter() methods
      //of any UI-enabled classes that were attached to the serialManager via the setupSerialManager() 
      //function used back in the main *.ino file.
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

  //Add first page to GUI  (the indentation doesn't matter; it is only to help us see it better)
  page_h = myGUI.addPage("TrebleBoost wComp");
      
      //Add a card (button group) under the first page
      card_h = page_h->addCard("Digital Gain (dB)");
          card_h->addButton("Swipe for Comp Settings","","",12);  //This just displays a message.  No functionality.  Full Width.

      //Add card to this page for Highpass cutoff
      card_h = page_h->addCard("Highpass Cutoff (Hz)");
          card_h->addButton("-","T","",        4);  //displayed string, command, button ID, button width (out of 12)
          card_h->addButton("", "", "cutoffHz",4);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
          card_h->addButton("+","t","",        4);  //displayed string, command, button ID, button width (out of 12)

  //Add a page for the left compressor's built-in GUI
  page_h = comp1.addPage_default(&myGUI);
  page_h->setName("Left Compressor");  //I'm overwriting the default name so that we know that this is the left WDRC compressor

  //Add a page for the right compressor's built-in GUI
  page_h = comp2.addPage_default(&myGUI);
  page_h->setName("Right Compressor");  //I'm overwriting the default name so that we know that this is the right WDRC compressor

  //Add a page for some other miscellaneous stuff
  page_h = myGUI.addPage("Globals");

    //Add an example card that just displays a value...no interactive elements
    card_h = page_h->addCard(String("Analog Input Gain (dB)"));
      card_h->addButton("", "", "inpGain", 12); //label, command, id, width (out of 12)...THIS IS FULL WIDTH!

    //Add a button group ("card") for the CPU reporting...use a button group that is built into myState for you!
    card_h = myState.addCard_cpuReporting(page_h);
        
  //add some pre-defined pages to the GUI (pages that are built-into the App)
  myGUI.addPredefinedPage("serialPlotter");  //if we send data in the right format, the App will plot the signal levels in real-time!
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
  
  //First, let's update the portion of the GUI that we are explicitly defining and controlling ourselves
  //here in the serial manager.
  updateFilterDisplay();
  setButtonText("inpGain",String(myState.input_gain_dB,1));

  //Then, let's have the system automatically update all of the individual UI elements that we attached
  //to the serialManager via the setupSerialManager() function used back in the main *.ino file.
  SerialManagerBase::setFullGUIState(activeButtonsOnly); //in here, it automatically loops over the different UI elements

}

void SerialManager::updateFilterDisplay(void) {
  setButtonText("cutoffHz", String(myState.cutoff_Hz,0));
}

void SerialManager::sendDataToAppSerialPlotter(const String data1, const String data2) {
  ble->sendMessage( String("P") + data1 + "," + data2 + String('\n') );
}


#endif
