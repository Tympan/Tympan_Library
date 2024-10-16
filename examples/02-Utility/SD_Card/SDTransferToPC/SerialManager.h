#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>

//Extern variables from the main *.ino file
extern Tympan myTympan;
extern SdFileTransfer sdFileTransfer;
extern void createDummyFileOnSD(void);

class SerialManager  { 
  public:
    SerialManager() {};

    void printHelp(void);
    bool respondToByte(char c);  //this is called automatically by SerialManagerBase.respondToByte(char c)
    int  receiveString(String &new_string,const unsigned long timeout_millis);
};

void SerialManager::printHelp(void) {  
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println(" General: No Prefix");
  Serial.println("   h    : Print this help");
  Serial.println("   d    : Create a dummy text file on the SD");
  Serial.println("   L    : Get a list of the files on SD");
  Serial.println("   t    : Transfer file from Tympan SD to PC via Serial ('send' interactive)");
  Serial.println("   T    : Transfer file from PC to Tympan SD via Serial ('receive' interactive)");
  Serial.println();
}

//switch yard to determine the desired action
bool SerialManager::respondToByte(char c) { 
  bool ret_val = true; //assume at first that we will find a match
  switch (c) {
    case 'h': 
      printHelp(); 
      break;
    case 'd':
      createDummyFileOnSD();
      break;
    case 'L':
      Serial.print("SerialMonitor: Listing Files on SD: "); //purposely don't include end-of-line
      sdFileTransfer.sendFilenames(','); //send file names seperated by commas
      break;
    case 't':
      if ((Serial.peek() == '\n') || (Serial.peek() == '\r')) Serial.read();  //remove any trailing EOL character
      sdFileTransfer.sendFile_interactive();
      break;
    case 'T':
      if ((Serial.peek() == '\n') || (Serial.peek() == '\r')) Serial.read();  //remove any trailing EOL character
      sdFileTransfer.receiveFile_interactive();
      break;
    default:
      //received unknown command.  Ignore if carrige return or newline.  Otherwise, notify the user.
      if ( (c != '\r') && (c != '\n') ) Serial.println("SerialMonitor: command " + String(c) + " not known");
      break;
  }
  return ret_val;
}

// int SerialManager::receiveString(String &new_string,const unsigned long timeout_millis) {
//   Serial.setTimeout(timeout_millis);  //set timeout in milliseconds
//   new_string.remove(0); //clear the string
//   new_string += Serial.readStringUntil('\n');  //append the string
//   if (new_string.length() == 0) new_string += Serial.readStringUntil('\n');  //append the string
//   Serial.setTimeout(1000);  //return the timeout to the default
//   return 0;
// }





#endif
