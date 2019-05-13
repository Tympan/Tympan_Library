
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>

//Extern variables
extern Tympan myTympan;
extern AudioSDWriter_F32 audioSDWriter;
extern float input_gain_dB;
extern State_t myState;
extern const int INPUT_PCBMICS;
extern const int INPUT_MICJACK;
extern const int INPUT_LINEIN_SE;
extern const int INPUT_LINEIN_JACK;


//Extern Functions
extern void setConfiguration(int);
extern void togglePrintMemoryAndCPU(void);
extern void setPrintMemoryAndCPU(bool);
extern void incrementInputGain(float);

//now, define the Serial Manager class
class SerialManager {
  public:
    SerialManager(void) {  };

    void respondToByte(char c);
    void printHelp(void);
    void printFullGUIState(void);
    void printGainSettings(void);
    void setButtonState(String btnId, bool newState);
    float gainIncrement_dB = 2.5f;

  private:

};

void SerialManager::printHelp(void) {
  myTympan.println();
  myTympan.println("SerialManager Help: Available Commands:");
  //myTympan.println("   J: Print the JSON config object, for the Tympan Remote app");
  //myTympan.println("    j: Print the button state for the Tympan Remote app");
  myTympan.println("   c: Start printing of CPU and Memory usage");
  myTympan.println("   C: Stop printing of CPU and Memory usage");
  myTympan.println("   w: Switch Input to PCB Mics");
  myTympan.println("   W: Switch Input to Headset Mics");
  myTympan.println("   e: Switch Input to LineIn on the Mic Jack");
  myTympan.print  ("   i: Input: Increase gain by "); myTympan.print(gainIncrement_dB); myTympan.println(" dB");
  myTympan.print  ("   I: Input: Decrease gain by "); myTympan.print(gainIncrement_dB); myTympan.println(" dB");
  myTympan.println("   p: SD: prepare for recording");
  myTympan.println("   r: SD: begin recording");
  myTympan.println("   s: SD: stop recording");
  myTympan.println("   h: Print this help");
  myTympan.println();
}


//switch yard to determine the desired action
void SerialManager::respondToByte(char c) {
  switch (c) {
    case 'h': case '?':
      printHelp(); break;
    case 'c':
      myTympan.println("Received: start CPU reporting");
      setPrintMemoryAndCPU(true);
      setButtonState("cpuStart",true);
      break;
    case 'C':
      myTympan.println("Received: stop CPU reporting");
      setPrintMemoryAndCPU(false);
      setButtonState("cpuStart",false);
      break;
    case 'i':
      incrementInputGain(gainIncrement_dB);
      printGainSettings();
      break;
    case 'I':   //which is "shift i"
      incrementInputGain(-gainIncrement_dB);
      printGainSettings();
      break;
    case 'w':
      myTympan.println("Received: PCB Mics");
      setConfiguration(INPUT_PCBMICS);
      setButtonState("configPCB",true);
      setButtonState("configHeadset",false);
      break;
    case 'W':
      myTympan.println("Recevied: Headset Mics.");
      setConfiguration(INPUT_MICJACK);
      setButtonState("configPCB",false);
      setButtonState("configHeadset",true);
      break;
    case 'e':
      myTympan.println("Received: Line-In on Jack");
      setConfiguration(INPUT_LINEIN_JACK);
      setButtonState("configPCB",false);
      setButtonState("configHeadset",false);     
    case 'p':
      myTympan.println("Received: prepare SD for recording");
      //prepareSDforRecording();
      audioSDWriter.prepareSDforRecording();
      break;
    case 'r':
      myTympan.println("Received: begin SD recording");
      //beginRecordingProcess();
      audioSDWriter.startRecording();
      setButtonState("recordStart",true);
      break;
    case 's':
      myTympan.println("Received: stop SD recording");
      audioSDWriter.stopRecording();
      setButtonState("recordStart",false);
      break;
    case 'J':
      {
        // Print the layout for the Tympan Remote app, in a JSON-ish string
        // (single quotes are used here, whereas JSON spec requires double quotes.  The app converts ' to " before parsing the JSON string).
        // If you don't give a button an id, then it will be assigned the id 'default'.  IDs don't have to be unique; if two buttons have the same id,
        // then you always control them together and they'll always have the same state. 
        // Please don't put commas or colons in your ID strings!
        // The 'icon' is how the device appears in the app - could be an icon, could be a pic of the device.  Put the
        // image in the TympanRemote app in /src/assets/devIcon/ and set 'icon' to the filename.
        char jsonConfig[] = "JSON={"
          "'icon':'tympan.png',"
          "'pages':["
            "{'title':'Presets','cards':["
              "{'name':'Record Mics to SD Card','buttons':[{'label': 'Start', 'cmd': 'r', 'id':'recordStart'},{'label': 'Stop', 'cmd': 's'}]}"
            "]},"
            "{'title':'Tuner','cards':["
              "{'name':'Select Input','buttons':[{'label': 'Headset Mics', 'cmd': 'W', 'id':'configHeadset'},{'label': 'PCB Mics', 'cmd': 'w', 'id': 'configPCB'}]},"
              "{'name':'Input Gain', 'buttons':[{'label': 'Less', 'cmd' :'I'},{'label': 'More', 'cmd': 'i'}]},"
              "{'name':'Record Mics to SD Card','buttons':[{'label': 'Start', 'cmd': 'r', 'id':'recordStart'},{'label': 'Stop', 'cmd': 's'}]},"
              "{'name':'CPU Reporting', 'buttons':[{'label': 'Start', 'cmd' :'c','id':'cpuStart'},{'label': 'Stop', 'cmd': 'C'}]}"
            "]}"                            
          "]"
        "}";
        myTympan.println(jsonConfig);
        delay(50);
        printFullGUIState();
        break;
      }
  }
}

void SerialManager::printFullGUIState(void) {
  switch (myState.input_source) {
    case (INPUT_PCBMICS):
      setButtonState("configPCB",true); delay(10);
      setButtonState("configHeadset",false);
      break;
    case (INPUT_MICJACK): 
      setButtonState("configPCB",false); delay(10);
      setButtonState("configHeadset",true);
      break;
    default:
      setButtonState("configPCB",false); delay(10);
      setButtonState("configHeadset",false);
      break;     
  }
}

void SerialManager::printGainSettings(void) {
  myTympan.print("Input PGA = ");
  myTympan.print(input_gain_dB, 1);
  myTympan.println();
}

void SerialManager::setButtonState(String btnId, bool newState) {
  if (newState) {
    myTympan.println("STATE=BTN:" + btnId + ":1");
  } else {
    myTympan.println("STATE=BTN:" + btnId + ":0");
  }
}

#endif
