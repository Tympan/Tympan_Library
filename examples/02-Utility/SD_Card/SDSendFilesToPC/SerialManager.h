
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>

//Extern variables from the main *.ino file
extern Tympan myTympan;
extern SDtoSerial SD_to_serial;
extern void createDummyFileOnSD(void);

class SerialManager  { 
  public:
    SerialManager() {};

    void printHelp(void);
    bool respondToByte(char c);  //this is called automatically by SerialManagerBase.respondToByte(char c)
    int receiveFilename(String &filename,const unsigned long timeout_millis);
};

void SerialManager::printHelp(void) {  
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println(" General: No Prefix");
  Serial.println("   h    : Print this help");
  Serial.println("   d    : Create a dummy text file on the SD");
  Serial.println("SerialManager: Typical file transfer procedure:");
  Serial.println("   L    : First, Get a list of the files on SD");
  Serial.println("   f    : Second, Open the file from SD (will prompt you for filename)");
  Serial.println("   b    : Third, Get the size of the file in bytes");
  Serial.println("   t    : Fourth, Transfer the whole file from SD to Serial");

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
      createDummyFileOnSD(); //this function is in the main *.ino file
      break;
    case 'L':
      Serial.print("SerialMonitor: Listing Files on SD:"); //don't include end-of-line
      SD_to_serial.sendFilenames(','); //send file names seperated by commas
      break;
    case 'f':
      {
        Serial.println("SerialMonitor: Opening file: Send filename (ending with newline character) within 10 seconds");
        String fname;  receiveFilename(fname, 10000);  //wait 10 seconds
        if (SD_to_serial.open(fname)) {
          Serial.println("SerialMonitor: " + fname + " successfully opened");
        } else {
          Serial.println("SerialMonitor: *** ERROR ***: " + fname + " could not be opened");
        }
      }
      break;
    case 'b':
      if (SD_to_serial.isFileOpen()) {
        SD_to_serial.sendFileSize();
      } else {
        Serial.println("SerialMonitor: *** ERROR ***: Cannot get file size because no file is open");
      }
      break;
    case 't':
      if (SD_to_serial.isFileOpen()) {
        SD_to_serial.sendFile();
        Serial.println();
      } else {
        Serial.println("SerialMonitor: *** ERROR ***: Cannot send file because no file is open");
      }
      break;
    default:
      //received unknown command.  Ignore if carrige return or newline.  Otherwise, notify the user.
      if ( (c != '\r') && (c != '\n') ) Serial.println("SerialMonitor: command " + String(c) + " not known");
      break;
  }
  return ret_val;
}

int SerialManager::receiveFilename(String &filename,const unsigned long timeout_millis) {
  Serial.setTimeout(timeout_millis);  //set timeout in milliseconds
  filename.remove(0); //clear the string
  filename += Serial.readStringUntil('\n');  //append the string
  if (filename.length() == 0) filename += Serial.readStringUntil('\n');  //append the string
  Serial.setTimeout(1000);  //return the timeout to the default
  return 0;
}





#endif
