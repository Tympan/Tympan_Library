
/* 
 *  SerialManager
 *  
 *  Created: Chip Audette, OpenAudio, April 2017
 *  
 *  Purpose: This class receives commands coming in over the serial link.
 *     It allows the user to control the Tympan via text commands, such 
 *     as changing the volume or changing the filter cutoff.
 *  
 */


#ifndef _SerialManager_h
#define _SerialManager_h

#include "AudioPaths.h"

//variables in the main sketch that I want to call from here
extern Tympan myTympan;
extern AudioSettings_F32 audio_settings;
extern AudioInputI2S_F32 audioInput;
extern AudioOutputI2S_F32 audioOutput;
extern AudioPath_Base *currentAudioPath;

//functions in the main sketch that I want to call from here
extern void togglePrintMemoryAndCPU(void);

//now, define the Serial Manager class
class SerialManager {
public:
  SerialManager(void){};

  void respondToByte(char c);
  void printHelp(void);
private:
};

void SerialManager::printHelp(void) {
  Serial.println();
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println("   h: Print this help");
  Serial.println("   C: Toggle printing of CPU and Memory usage");
  Serial.println("   0: Delete current audio path");
  Serial.println("   1: Delete current audio path and make Audio Path 1");
  Serial.println("   2: Delete current audio path and make Audio Path 2");
  Serial.println();
}

//switch yard to determine the desired action
void SerialManager::respondToByte(char c) {
  switch (c) {
    case 'h':
    case '?':
      printHelp();
      break;
    case 'C':
    case 'c':
      Serial.println("Command Received: toggle printing of memory and CPU usage.");
      togglePrintMemoryAndCPU();
      break;
    case '0':
      if (currentAudioPath != NULL) {
        Serial.println("SerialManager: Received 0: deleting existing audio path...");
        delete currentAudioPath;  currentAudioPath = NULL;
      } else {
        Serial.println("SerialManager: Received 0: no existing audio path to delete.");
      }
      //myTympan.printCPUandMemoryMessage();Serial.flush();
      break;
    case '1':
      Serial.println("SerialManager: Received 1: switch to AudioPath_Sine...");
      if (currentAudioPath != NULL) { delete currentAudioPath; currentAudioPath = NULL; }
      currentAudioPath = new AudioPath_Sine(audio_settings, &audioInput, &audioOutput);
      //myTympan.printCPUandMemoryMessage();Serial.flush();
      break;
    case '2':
      Serial.println("SerialManager: Received 2: switch to AudioPath_PassThruGain...");
      if (currentAudioPath != NULL) { delete currentAudioPath;  currentAudioPath = NULL; }
      currentAudioPath = new AudioPath_PassThruGain(audio_settings, &audioInput, &audioOutput);
      //myTympan.printCPUandMemoryMessage();Serial.flush();
      break;
  }
}


#endif
