
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>


//Extern variables from the main *.ino file
extern Tympan myTympan;
extern AudioSDPlayer_F32 audioSDPlayer;
extern AudioSDWriter_F32 audioSDWriter;

String sdPlay_filename = "PLAY1.WAV";

class SerialManager : public SerialManagerBase  {  // see Tympan_Library for SerialManagerBase for more functions!
  public:
    SerialManager() : SerialManagerBase() {};

    void printHelp(void);
    void createTympanRemoteLayout(void) {}; //placeholder 
    void printTympanRemoteLayout(void) {};  //placeholder
    bool processCharacter(char c);  //this is called automatically by SerialManagerBase.respondToByte(char c)
};

void SerialManager::printHelp(void) {  
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println(" General: No Prefix");
  Serial.println("   h    : Print this help");
  Serial.println("   p    : SDPlay : Play file from SD card named " + sdPlay_filename);
  Serial.println("   q    : SDPlay : Stop any currnetly plying SD file");
  Serial.println("   r    : SDWrite: Manually start recording");
  Serial.println("   s    : SDWrite: Manually stop recording");
  
  //Add in the printHelp() that is built-into the other UI-enabled system components.
  //The function call below loops through all of the UI-enabled classes that were
  //attached to the serialManager in the setupSerialManager() function used back
  //in the main *.ino file.
  SerialManagerBase::printHelp();  ////in here, it automatically loops over the different UI elements issuing printHelp()
  
  Serial.println();
}

//switch yard to determine the desired action
bool SerialManager::processCharacter(char c) { //this is called by SerialManagerBase.respondToByte(char c)
  bool ret_val = true; //assume at first that we will find a match
  switch (c) {
    case 'h': 
      printHelp(); 
      break;
    case 'p':
      Serial.println("Received: starting SD Player for " + sdPlay_filename + "...");
      audioSDPlayer.play(sdPlay_filename);
      break; 
    case 'q':
      Serial.println("Received: stopping the playing of any SD files...");
      audioSDPlayer.stop();
      break;
    case 'r':
      audioSDWriter.startRecording();
      Serial.println("Starting recording to SD file " + audioSDWriter.getCurrentFilename());
      break;
    case 's':
      Serial.println("Stopping recording to SD file " + audioSDWriter.getCurrentFilename());
      audioSDWriter.stopRecording();
      break;
    default:
      ret_val = SerialManagerBase::processCharacter(c);  //in here, it automatically loops over the different UI elements
      break;
  }
  return ret_val;
}



#endif
