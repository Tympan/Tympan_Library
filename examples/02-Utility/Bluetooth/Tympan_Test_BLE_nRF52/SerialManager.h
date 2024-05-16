
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>
#include "State.h"

//classes from the main sketch that might be used here
extern Tympan myTympan;                  //created in the main *.ino file
extern State myState;                    //created in the main *.ino file
extern AudioSettings_F32 audio_settings; //created in the main *.ino file  
extern BLE_nRF52 &ble;


//functions in the main sketch that I want to call from here
extern void changeGain(float);
extern void printGainLevels(void);

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

void setButtonState(String btnId, bool newState) {
  String msg = String("STATE=BTN:" + btnId + ":1");
  //Serial.println("serialManager: setButtonState: sending = " + msg); //echo to USB Serial for debugging
  ble.sendMessage(msg);
}
void setButtonText(String btnId, String text) {
  String msg = String("TEXT=BTN:" + btnId + ":" + text);
  //Serial.println("serialManager: setButtonText: sending = " + msg); //echo to USB Serial for debugging
  ble.sendMessage(msg);
}


//now, define the Serial Manager class
class SerialManager  {  // see Tympan_Library for SerialManagerBase for more functions!
  public:
    SerialManager(void)  {};
      
    void printHelp(void);
    void createTympanRemoteLayout(void); 
    void printTympanRemoteLayout(void); 
    bool respondToByte(char c);
    bool processCharacter(char c);  //this is called automatically by SerialManagerBase.respondToByte(char c)

    //method for updating the GUI on the App
    void setFullGUIState(bool activeButtonsOnly = false);
    void updateGainDisplay(void);
    void updateCpuDisplayOnOff(void);
    void updateCpuDisplayUsage(void);

    //factors by which to raise or lower the parameters when receiving commands from TympanRemote App
    float gainIncrement_dB = 3.0f;            //raise or lower by x dB
  private:
    TympanRemoteFormatter myGUI;  //Creates the GUI-writing class for interacting with TympanRemote App
   
};

void SerialManager::printHelp(void) {  
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println(" h: Print this help");
  Serial.println(" k/K: AUDIO: Incr/Decrease Digital Gain");
  Serial.println(" c/C: SYSTEM: Enable/Disable printing of CPU and Memory usage");
  Serial.println(" v:   BLE: Get 'Version of firmware from module");
  Serial.println(" n:   BLE: Get 'Name' from module");
  Serial.println(" N:   BLE: Set 'Name' of module to 'TYMP-TYMP'");
  Serial.println(" t:   BLE: Get 'Advertising' status via software");
  Serial.println(" f/F: BLE: Set 'Advertising' to ON (f) or OFF (F)");
  Serial.println(" g/G: BLE: Get 'Connected' status via software (g) or GPIO (G)");
  Serial.println(" m:   BLE: Get 'LED Mode' setting (0=OFF, 1=AUTO)");
  Serial.println(" b/B: BLE: Set 'LED Mode' to AUTO (b) or OFF (B)");
  Serial.println(" J:   Send JSON for the GUI for the Tympan Remote App");
  Serial.println();
}

bool SerialManager::respondToByte(char c) {
  //Serial.println("SerialManager: respondToByte " + String(c));
  return processCharacter(c);
}

//switch yard to determine the desired action
bool SerialManager::processCharacter(char c) {  //this is called by SerialManagerBase.respondToByte(char c)
  bool ret_val = true;
  int val = 0;
  int err_code = 0;

  switch (c) {
    case 'h':
      printHelp(); 
      break;
   case 'k':
      changeGain(gainIncrement_dB);   //raise
      printGainLevels();      //print to USB Serial (ie, to the SerialMonitor)
      updateGainDisplay();    //update the App
      break;
    case 'K':
      changeGain(-gainIncrement_dB);  //lower
      printGainLevels();     //print to USB Serial (ie, to the SerialMonitor)
      updateGainDisplay();   //update the App
      break;
    case 'c':
      Serial.println("Starting CPU reporting...");
      myState.printCPUtoGUI = true;
      updateCpuDisplayOnOff();
      break;
    case 'C':
      Serial.println("Stopping CPU reporting...");
      myState.printCPUtoGUI = false;
      updateCpuDisplayOnOff();
      break;  
    case 'n':
      {
        String name = String("");
        int err_code = ble.getBleName(&name);
        Serial.println("serialManager: BLE: Name from module = " + name);
        if (err_code != 0) Serial.println("serialManager:  ble.getBleName returned error code " + String(err_code));
        setButtonText("bleName",name);
      }
      break;
    case 'N':
      {
        String name = String("TympTymp");
        Serial.println("serialManager: BLE: Set Name of module to " + name);
        int err_code = ble.setBleName(name);
        if (err_code != 0) Serial.println("serialManager:  ble.setBleName returned error code " + String(err_code));
      }
      break;
    case 't':
      val = ble.isAdvertising();
      Serial.println("serialManager: BLE: Advertising = " + String(val));
      setButtonText("isAdvert", String(val ? "ON" : "OFF"));
      break;
    case 'f':
      Serial.println("serialManager: BLE: Set Advertising to ON...");
      err_code = ble.enableAdvertising(true);
      if (err_code) Serial.println("SerialManager: ble.enableAdvertising returned err_code = " + String(err_code));
      break;
    case 'F':
      Serial.println("serialManager: BLE: Set Advertising to OFF...");
      err_code = ble.enableAdvertising(false);
      if (err_code) Serial.println("SerialManager: ble.enableAdvertising returned err_code = " + String(err_code));
      break;
    case 'm':
      val = ble.getLedMode();
      if (val < 0) {
        Serial.println("SerialManager: ble.getLedMode returned err_code = " + String(val));
      } else {
        Serial.println("serialManager: BLE: LED Mode = " + String(val) + " (1=AUTO, 0=OFF)");
        setButtonText("ledMode", String(val ? "AUTO" : "OFF"));
      }
      break;
    case 'b':
      Serial.println("serialManager: BLE: Set LED Mode to 1 (AUTO)");
      err_code = ble.setLedMode(1);
      if (err_code < 0) {
        Serial.println("SerialManager: ble.setLedMode returned err_code = " + String(err_code));
      } else {
        delay(5); 
        Serial.println("serialManager: following up by issuing ble.getLedMode()...");
        val = ble.getLedMode();  
        if (val < 0) {
          Serial.println("serialManager: ble.getLedMode returned err_code = " + String(val));  
        } else {
          Serial.println("serialManager: BLE: LED Mode = " + String(val) + " (1=AUTO, 0=OFF)");
          setButtonText("ledMode", String(val ? "AUTO" : "OFF"));
        }
      }
      break;
    case 'B':
      Serial.println("serialManager: BLE: Set LED Mode to 0 (OFF)");
      err_code = ble.setLedMode(0);
      if (err_code < 0) {
        Serial.println("SerialManager: ble.setLedMode returned err_code = " + String(err_code));
      } else {
        delay(5); 
        Serial.println("serialManager: following up by issuing ble.getLedMode()...");
        val = ble.getLedMode();  
        if (val < 0) {
          Serial.println("SerialManager: ble.getLedMode returned err_code = " + String(val));  
        } else {
          Serial.println("serialManager: BLE: LED Mode = " + String(val) + " (1=AUTO, 0=OFF)");
          setButtonText("ledMode", String(val ? "AUTO" : "OFF"));
        }
      }  
      break; 
    case 'g':
      val = ble.isConnected(BLE_nRF52::GET_VIA_SOFTWARE);
      if (val < 0) {
        Serial.println("SerialManager: ble.isConnected(Software) returned err_code = " + String(val));
      } else {
        Serial.println("serialManager: BLE: Connected (via software) = " + String(val));
        setButtonText("isConn_s", String(val ? "YES" : "NO"));
        setButtonText("isConn_g", String(""));
      }
      break;
    case 'G':
      val = ble.isConnected(BLE_nRF52::GET_VIA_GPIO);
      if (val < 0) {
        Serial.println("SerialManager: ble.isConnected(GPIO) returned err_code = " + String(val));
      } else {
        Serial.println("serialManager: BLE: Connected (via GPIO) = " + String(val));
        setButtonText("isConn_g", String(val ? "YES" : "NO"));
        setButtonText("isConn_s", String(""));
      }
      break;
    case 'v':
      {
        String version;
        err_code = ble.version(&version);
        if (err_code < 0) {
          Serial.println("serialManager: BLE: version returned err_code " + String(err_code));
        } else {
          Serial.println("serialManager: BLE module firmware: " + version);
          setButtonText("version",version);
        }
      }
      break; 
    case 'J': case 'j':           //The TympanRemote app sends a 'J' to the Tympan when it connects
      printTympanRemoteLayout();  //in resonse, the Tympan sends the definition of the GUI that we'd like
      break;
    default:
      ret_val = false;
  }
  return ret_val;
}


// //////////////////////////////////  Methods for defining and transmitting the GUI to the App

void SerialManager::createTympanRemoteLayout(void) {
  
  // Create some temporary variables
  TR_Page *page_h;  //dummy handle for a page
  TR_Card *card_h;  //dummy handle for a card

  //Add first page to GUI  (the indentation doesn't matter; it is only to help us see it better)
  page_h = myGUI.addPage("nRF52 BLE Testing");
      //Add a card to this page
      card_h = page_h->addCard("BLE Name");
          card_h->addButton("Get Name","n",""     , 12);
          card_h->addButton("", "", "bleName", 12);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)

      //Add a card to this page
      card_h = page_h->addCard("BLE Firmware");
          card_h->addButton("Get Version","v","", 12);
          card_h->addButton("",  "", "version",   12);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)

      //Add a card to this page
      card_h = page_h->addCard("Bluetooth LED");
          card_h->addButton("Get Mode","m",""     , 6);
          card_h->addButton("", "", "ledMode", 6);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
          card_h->addButton("AUTO On","b","",     6);  //displayed string, command, button ID, button width (out of 12)
          card_h->addButton("AUTO Off","B","",      6);  //displayed string, command, button ID, button width (out of 12)
          
  //Add a second page to the GUI
  page_h = myGUI.addPage("Audio");

    //Add an example card that just displays a value...no interactive elements
    card_h = page_h->addCard(String("Analog Input Gain (dB)"));
      card_h->addButton("",      "", "inpGain",   12); //label, command, id, width (out of 12)...THIS ISFULL WIDTH!

    card_h = page_h->addCard("Digital Gain (dB)");
        card_h->addButton("-","K","",           4);  //displayed string, command, button ID, button width (out of 12)
        card_h->addButton("","","gainIndicator",4);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
        card_h->addButton("+","k","",           4);   //displayed string, command, button ID, button width (out of 12)


    //Add an example card where one of the buttons will indicate "on" or "off"
//    card_h = page_h->addCard(String("CPU Usage (%)"));
//      card_h->addButton("Start", "c", "cpuStart", 4);  //label, command, id, width...we'll light this one up if we're showing CPU usage
//      card_h->addButton(""     , "",  "cpuValue", 4);  //label, command, id, width...this one will display the actual CPU value
//      card_h->addButton("Stop",  "C", "",         4);  //label, command, id, width...this one will turn off the CPU display
        
  //add some pre-defined pages to the GUI (pages that are built-into the App)
  myGUI.addPredefinedPage("serialMonitor");
  //myGUI.addPredefinedPage("serialPlotter");
}


// Print the layout for the Tympan Remote app, in a JSON-ish string
void SerialManager::printTympanRemoteLayout(void) {
    Serial.println("SerialManager: printTympanRemoteLayout: sending JSON...");
    if (myGUI.get_nPages() < 1) createTympanRemoteLayout();  //create the GUI, if it hasn't already been created
    String s = myGUI.asString();
    Serial.println(s);
    ble.sendMessage(s); //ble is held by SerialManagerBase
    setFullGUIState();
}

// //////////////////////////////////  Methods for updating the display on the GUI

void SerialManager::setFullGUIState(bool activeButtonsOnly) {
  updateGainDisplay();
  setButtonText("inpGain",String(myState.input_gain_dB,1));
  updateCpuDisplayOnOff();
}

void SerialManager::updateGainDisplay(void) {
  setButtonText("gainIndicator", String(myState.digital_gain_dB,1));
}


void SerialManager::updateCpuDisplayOnOff(void) {
  //setButtonState("cpuStart",myState.printCPUtoGUI);  //illuminate the button if we will be sending the CPU value
}

void SerialManager::updateCpuDisplayUsage(void) {
   setButtonText("cpuValue",String(audio_settings.processorUsage(),1)); //one decimal places
}

#endif
