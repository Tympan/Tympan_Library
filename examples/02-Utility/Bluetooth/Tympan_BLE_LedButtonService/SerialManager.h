
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>

//classes from the main sketch that might be used here
extern Tympan myTympan;                  //created in the main *.ino file
extern AudioSettings_F32 audio_settings; //created in the main *.ino file  


//functions in the main sketch that I want to call from here
extern void changeGain(float);
extern void printGainLevels(void);

//now, define the Serial Manager class
class SerialManager : virtual public SerialManagerBase {  // see Tympan_Library for SerialManagerBase for more functions!
  public:
    SerialManager(BLE *_ble): SerialManagerBase(_ble)  {};
      
    void printHelp(void);
    bool processCharacter(char c);  //this is called automatically by SerialManagerBase.respondToByte(char c)

  private:
   
};

void SerialManager::printHelp(void) {  
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println(" h: Print this help");

  Serial.println();
}

//switch yard to determine the desired action
bool SerialManager::processCharacter(char c) {  //this is called by SerialManagerBase.respondToByte(char c)
  bool ret_val = true;
  //int val = 0;
  //int err_code = 0;

  switch (c) {
    case 'h':
      printHelp(); 
      break;
  
    default:
      ret_val = false;
  }
  return ret_val;
}



#endif
