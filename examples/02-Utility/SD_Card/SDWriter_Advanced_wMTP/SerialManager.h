
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>
#include "State.h"

//Extern variables from the main *.ino file
extern Tympan myTympan;
extern AudioSDWriter_F32 audioSDWriter;
extern State myState;

//Extern Functions
extern void setConfiguration(int);
extern float incrementInputGain(float);

//externals for MTP
extern void start_MTP(void);
//extern void stop_MTP(void);

class SerialManager {  // see Tympan_Library for SerialManagerBase for more functions!
  public:
    SerialManager() {};

    void printHelp(void);
    bool respondToByte(char c); 

    int gainIncrement_dB = 2.5;

};

void SerialManager::printHelp(void) {  
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println(" General: No Prefix");
  Serial.println("   h: Print this help");
  Serial.println("   w: Switch Input to PCB Mics");
  Serial.println("   W: Switch Input to Headset Mics");
  Serial.println("   e: Switch Input to LineIn on the Mic Jack");
  Serial.println("   i: Input: Increase gain by " + String(gainIncrement_dB) + " dB");
  Serial.println("   I: Input: Decrease gain by " + String(gainIncrement_dB) + " dB");
  Serial.println("   r: Start Recording to SD");
  Serial.println("   s: Stop Recording to SD");
  #if defined(USE_MTPDISK) || defined(USB_MTPDISK_SERIAL)  //detect whether "MTP Disk" or "Serial + MTP Disk" were selected in the Arduino IDE
    #if defined(__IMXRT1062__)
      //only for Teensy 4
      Serial.println("   |: Reboot the headset (do before entering MTP mode)");
    #endif
    Serial.println("   >: Start MTP mode to read SD from PC");
  #endif

  Serial.println();
}


//switch yard to determine the desired action
bool SerialManager::respondToByte(char c) {
  bool ret_val = true; //assume at first that we will find a match
  switch (c) {
    case 'h': 
      printHelp(); 
      break;  
    case 'i':
      incrementInputGain(gainIncrement_dB);
      Serial.println("Received: Increased input gain to: " + String(myState.input_gain_dB) + " dB");
      break;
    case 'I':   //which is "shift i"
      incrementInputGain(-gainIncrement_dB);
      Serial.println("Received: Decreased input gain to: " + String(myState.input_gain_dB) + " dB");
      break;
    case 'w':
      Serial.println("Received: Switch input to PCB Mics");
      setConfiguration(State::INPUT_PCBMICS);
      break;
    case 'W':
      Serial.println("Recevied: Switch input to Mics on Jack.");
      setConfiguration(State::INPUT_JACK_MIC);
      break;
    case 'e':
      myTympan.println("Received: Switch input to Line-In on Jack");
      setConfiguration(State::INPUT_JACK_LINE);
      break;
    case 'r':
      audioSDWriter.startRecording();
      Serial.println("Starting recording to SD file " + audioSDWriter.getCurrentFilename());
      break;
    case 's':
      Serial.println("Stopping recording to SD file " + audioSDWriter.getCurrentFilename());
      audioSDWriter.stopRecording();
      break;
    case '|':
      #if defined(__IMXRT1062__)
        Serial.println("Rebooting the Tympan (it might take 15 seconds)"); Serial.flush();
        audioSDWriter.stopRecording(); delay(250);
        SCB_AIRCR = 0x05FA0004; //for Teensy 4 https://forum.pjrc.com/index.php?threads/how-to-trigger-software-reset-on-teensy-4-1.69849/
      #endif
      break;
    case '>':
      Serial.println("Starting MTP service to access SD card (everything else will stop)");
      start_MTP();
      break;
  }
  return ret_val;
}




#endif
