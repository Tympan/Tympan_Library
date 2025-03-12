
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>

//classes from the main sketch that might be used here
extern Tympan myTympan;                  //created in the main *.ino file
extern AudioSettings_F32 audio_settings; //created in the main *.ino file  
extern int ble_service_id;
extern BLE_nRF52& ble_nRF52;

//now, define the Serial Manager class
class SerialManager : virtual public SerialManagerBase {  // see Tympan_Library for SerialManagerBase for more functions!
  public:
    SerialManager(BLE *_ble): SerialManagerBase(_ble)  {};
      
    void printHelp(void);
    bool processCharacter(char c);  //this is called automatically by SerialManagerBase.respondToByte(char c)

  private:
   
};

void SerialManager::printHelp(void) {  
  Serial.println("SerialManager: Tympan_BLE_CustomService.ino:");
  Serial.println("   : Connect to Tympan using nRF Connect phone app");
  Serial.println("   : Once connected, you can send data from the phone to the Tympan at");
  Serial.println("     any time and it will be displayed here in the Serial Monitor.");
  Serial.println("   : Once connected, you can send data from the Tympan to the phone");
  Serial.println("     using the SerialManager commands below.");
  Serial.println();
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println("  h: Print this help");
  Serial.println("  1: Send 'a' to BLE Char 0 (if it exists and is allowed)");
  Serial.println("  2: Send 1234 as uint32 to BLE Char 1 (if it exists and is allowed)");
  Serial.println();
}

//switch yard to determine the desired action
bool SerialManager::processCharacter(char c) {  //this is called by SerialManagerBase.respondToByte(char c)
  bool ret_val = true;
  int ble_char_id;

  switch (c) {
    case 'h':
      printHelp(); 
      break;
    case '1':
      ble_char_id = 0;
      Serial.println("SerialManager: sending 'a' to BLE...");
      ble_nRF52.notifyBle(ble_service_id, ble_char_id, (uint8_t)'a');  //notifyBle() has this uint8 version that we can invoke
      break;
    case '2':
      ble_char_id = 1;
      Serial.println("SerialManager: sending (uint32_t)(1234) to BLE...");
      ble_nRF52.notifyBle(ble_service_id, ble_char_id, (uint32_t)1234);  //notifyBle() has this uint32 version that we can invoke
      break;
    default:
      ret_val = false;
  }
  return ret_val;
}



#endif
