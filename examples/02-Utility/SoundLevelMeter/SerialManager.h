
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>
#include "State.h"

//externally defined objects
extern Tympan                        myTympan;
extern State                         myState;
extern AudioCalcLevel_F32            calcLevel1;
extern AudioFilterFreqWeighting_F32  freqWeight1;
extern AudioCalcLeqCumulative_F32    calcLevelCumulative;

//functions in the main sketch that I want to call from here
extern bool enablePrintMemoryAndCPU(bool);
extern bool enablePrintLoudnessLevels(bool);
extern bool enablePrintingToBLE(bool);
extern bool enablePrintingToBLEplotter(bool);
extern int setFreqWeightType(int);
extern int setTimeAveragingType(int);

//Define a class to help manage the interactions with Serial comms (from SerialMonitor or from Bluetooth (BLE))
//see SerialManagerBase.h in the Tympan Library for some helpful supporting functions (like {"sendButtonState")
//https://github.com/Tympan/Tympan_Library/tree/master/src
class SerialManager : public SerialManagerBase {  
  public:
    SerialManager(BLE *_ble) : SerialManagerBase(_ble) { myGUI = new TympanRemoteFormatter; };
    void respondToByte(char c);
    void printHelp(void);

    //define the GUI for the App
    TympanRemoteFormatter *myGUI;  //Creates the GUI-writing class for interacting with TympanRemote App
    void createTympanRemoteLayout(void);
    void printTympanRemoteLayout(void);

    void updateFreqButtons(void);
    void updateTimeButtons(void);
    void updateControlButtons(void);
  private:

};

void SerialManager::printHelp(void) {
  myTympan.println();
  myTympan.println("SerialManager Help: Available Commands:");
  myTympan.println("   h:   Print this help");
  myTympan.println("   c,C: Enable/Disable printing of CPU and Memory usage");
  myTympan.println("   f,F: A-weight or C-weight");  
  myTympan.println("   t,T: FAST time constant or SLOW time constant");
  myTympan.println("   z  : Reset (zero) the cumulative Leq calculation");
  myTympan.println("   l,L: Enable/Disable printing of loudness level to USB");
  myTympan.println("   v,V: Start/Stop sending level to TympanRemote App.");
  myTympan.println("   ],}: Start/Stop sending level to TympanRemote App Plotter.");
  myTympan.println("   0:   Reset max loudness value.");
  myTympan.println();
}


//switch yard to determine the desired action
void SerialManager::respondToByte(char c) {
  //float old_val = 0.0, new_val = 0.0;
  switch (c) {
    case 'h': case '?':
      printHelp(); break;
    case 'c':
      myTympan.println("Command Received: enable printing of memory and CPU usage.");
      enablePrintMemoryAndCPU(true);
      break;
    case 'C':
      myTympan.println("Command Received: disable printing of memory and CPU usage.");
      enablePrintMemoryAndCPU(false);
      break;
    case 'f':
      myTympan.println("Command Received: setting to A-weight");
      setFreqWeightType(State::FREQ_A_WEIGHT);
      updateFreqButtons();
      break;      
    case 'F':
      myTympan.println("Command Received: setting to C-weight");
      setFreqWeightType(State::FREQ_C_WEIGHT);
      updateFreqButtons();      
      break;
    case 't':
      myTympan.println("Command Received: setting to FAST time constant");
      setTimeAveragingType(State::TIME_FAST);
      updateTimeButtons();
      break;      
    case 'z':
      myTympan.println("Command Received: resetting the cumulative Leq calculation");
      calcLevelCumulative.resetCumLeq();
      break;      
    case 'T':
      myTympan.println("Command Received: setting to SLOW time constant");
      setTimeAveragingType(State::TIME_SLOW);
      updateTimeButtons();
      break;      
    case 'l':
      myTympan.println("Command Received: enable printing of loudness levels to USB.");
      enablePrintLoudnessLevels(true);
      break;
    case 'L':
      myTympan.println("Command Received: disable printing of loudness levels to USB.");
      enablePrintLoudnessLevels(false);
      break;
    case 'v':
      myTympan.println("Command Received: enable printing of loudness levels to BLE.");
      enablePrintingToBLE(true);
      updateControlButtons();
      break;
    case 'V':
      myTympan.println("Command Received: disable printing of loudness levels to BLE.");
      enablePrintingToBLE(false);
      updateControlButtons();
      break;
    case ']':
      myTympan.println("Command Received: enable printing of loudness levels to BLE Plotter.");
      enablePrintingToBLEplotter(true);
      //updateControlButtons();
      break;
    case '}':
      myTympan.println("Command Received: disable printing of loudness levels to BLE Plotter.");
      enablePrintingToBLEplotter(false);
      //updateControlButtons();
      break;
    case '0':
      myTympan.println("Command Received: reseting max SPL.");
      calcLevel1.resetMaxLevel();
      break;     
    case 'J': case 'j':
      createTympanRemoteLayout();
      printTympanRemoteLayout();
      break; 
     
  }
};

void SerialManager::createTympanRemoteLayout(void) { 
  if (myGUI) delete myGUI; //delete any pre-existing GUI from memory (on the Tympan, not on the App)
  myGUI = new TympanRemoteFormatter(); 

  // Create some temporary variables
  TR_Page *page_h;  TR_Card *card_h; 

  //Add first page to GUI
  page_h = myGUI->addPage("Sound Level Meter");
      //Add card and buttons under the first page
      card_h = page_h->addCard("Frequency Weighting");
          card_h->addButton("A-Weight","f","Aweight",6);  //displayed string, command, button ID, button width (out of 12)
          card_h->addButton("C-Weight","F","Cweight",6);  //displayed string, command, button ID, button width (out of 12)
      card_h = page_h->addCard("Time Averaging");
          card_h->addButton("SLOW","T","slowTime",6);  //displayed string, command, button ID, button width (out of 12)
          card_h->addButton("FAST","t","fastTime",6);  //displayed string, command, button ID, button width (out of 12)
      card_h = page_h->addCard("Measured Loudness (dB SPL)");
          card_h->addButton("Now","","",6);  //displayed string, command, button ID, button width (out of 12)
          card_h->addButton("","","now",6);  //displayed string, command, button ID, button width (out of 12)
          card_h->addButton("Max","","",6);  //displayed string, command, button ID, button width (out of 12)
          card_h->addButton("","","max",6);  //displayed string, command, button ID, button width (out of 12)
          card_h->addButton("Start","v","start",6);
          card_h->addButton("Stop","V","stop",6); 
          card_h->addButton("Reset Max","0","resetMax",12);  //displayed string, command, button ID, button width (out of 12)
        
          
 
  //add some pre-defined pages to the GUI
  myGUI->addPredefinedPage("serialPlotter");  
  //myGUI->addPredefinedPage("serialMonitor");
}

void SerialManager::printTympanRemoteLayout(void) { 
  myTympan.println(myGUI->asString()); 
  ble->sendMessage(myGUI->asString());

  updateFreqButtons();
  updateTimeButtons();
  updateControlButtons();
}

void SerialManager::updateFreqButtons(void) {
  switch (myState.cur_freq_weight) {
    case State::FREQ_A_WEIGHT:
      setButtonState("Aweight",true);
      setButtonState("Cweight",false);
      break;
    case State::FREQ_C_WEIGHT:
      setButtonState("Aweight",false);
      setButtonState("Cweight",true);
      break;
  }
}

void SerialManager::updateTimeButtons(void) {
  switch (myState.cur_time_averaging) {
    case State::TIME_SLOW:
      setButtonState("slowTime",true);
      setButtonState("fastTime",false);
      break;
    case State::TIME_FAST:
      setButtonState("slowTime",false);
      setButtonState("fastTime",true);
      break;  
  }
}

void SerialManager::updateControlButtons(void) {
  if (myState.enable_printTextToBLE) {
    setButtonState("start",true);
  } else { 
    setButtonState("start",false);
  }
}

#endif
