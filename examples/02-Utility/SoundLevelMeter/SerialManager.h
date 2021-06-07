
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>

//externally defined objects
extern Tympan myTympan;
extern BLE ble;

//functions in the main sketch that I want to call from here
extern bool enablePrintMemoryAndCPU(bool);
extern bool enablePrintLoudnessLevels(bool);
extern bool enablePrintingToBLE(bool);

//now, define the Serial Manager class
class SerialManager {
  public:
    SerialManager(void) { myGUI = new TympanRemoteFormatter; };
    void respondToByte(char c);
    void printHelp(void);

    //define the GUI for the App
    TympanRemoteFormatter *myGUI;  //Creates the GUI-writing class for interacting with TympanRemote App
    void createTympanRemoteLayout(void);
    void printTympanRemoteLayout(void) { myTympan.println(myGUI->asString()); ble.sendMessage(myGUI->asString());}

  private:

};

void SerialManager::printHelp(void) {
  myTympan.println();
  myTympan.println("SerialManager Help: Available Commands:");
  myTympan.println("   h: Print this help");
  myTympan.println("   c,C: Enable/Disable printing of CPU and Memory usage");
  myTympan.println("   ],}: Start/Stop sending level to TympanRemote App.");
  myTympan.println("   l,L: Enable/Disable printing of loudness level");
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
    case 'l':
      myTympan.println("Command Received: enable printing of loudness levels.");
      enablePrintLoudnessLevels(true);
      break;
    case 'L':
      myTympan.println("Command Received: disable printing of loudness levels.");
      enablePrintLoudnessLevels(false);
      break;
    case ']':
      myTympan.println("Command Received: enable printing of loudness levels.");
      enablePrintingToBLE(true);
      enablePrintLoudnessLevels(true);
      break;
    case '}':
      myTympan.println("Command Received: disable printing of loudness levels.");
      enablePrintingToBLE(false);
      enablePrintLoudnessLevels(false);
      break;
    case 'J': case 'j':
      createTympanRemoteLayout();
      printTympanRemoteLayout();
      break;      
  }
};

void SerialManager::createTympanRemoteLayout(void) { 
  //if (myGUI) delete myGUI;
  //myGUI = new TympanRemoteFormatter();
  
  //add some pre-defined pages to the GUI
  myGUI->addPredefinedPage("serialPlotter");  
  myGUI->addPredefinedPage("serialMonitor");
}

#endif
