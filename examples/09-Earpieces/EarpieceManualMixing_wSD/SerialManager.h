#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>

//Extern variables...originally defined in the main *.ino file
extern Tympan myTympan;
extern AudioSDWriter_F32 audioSDWriter;

//Extern Functions
extern void setInputSource(Mic_Input);
extern void setInputMixer(Mic_Channels, int);
extern void incrementInputGain(float);
extern void incrementKnobGain(float);


//now, define the Serial Manager class
class SerialManager {
  public:
    SerialManager(void) {  };
      
    void respondToByte(char c);    
    void printHelp(void); 
    const float INCREMENT_INPUTGAIN_DB = 2.5f;  
    const float INCREMENT_HEADPHONE_GAIN_DB = 2.5f;  

};

void SerialManager::printHelp(void) {
  myTympan.println();
  myTympan.println("SerialManager Help: Available Commands:");
  myTympan.println("  h: Print this help");
  myTympan.println("  w: Inputs: Use PCB Mics");
  myTympan.println("  d: Inputs: Use PDM Mics as input");
  myTympan.println("  W: Inputs: Use Mic on Mic Jack");
  myTympan.println("  e: Inputs: Use LineIn on Mic Jack");
  myTympan.print  ("  i/I: Increase/Decrease Input Gain by "); myTympan.print(INCREMENT_INPUTGAIN_DB); myTympan.println(" dB");
  myTympan.print  ("  k/K: Increase/Decrease Headphone Volume by "); myTympan.print(INCREMENT_HEADPHONE_GAIN_DB); myTympan.println(" dB");
  myTympan.println("  r,s,|: SD: begin/stop/deleteAll recording");
  myTympan.println("  !/1: Enable/Disable Left-Front Earpiece Mic");
  myTympan.println("  @/2: Enable/Disable Left-Rear Earpiece Mic");
  myTympan.println("  #/3: Enable/Disable Right-Front Earpiece Mic");
  myTympan.println("  $/4: Enable/Disable Right-Rear Earpiece Mic");
  myTympan.println("  %/5: Enable/Disable All Earpiece Mics");
}


//switch yard to determine the desired action
void SerialManager::respondToByte(char c) {

  //Serial.println("SerialManager: respondToByte: received character: " + String(c));
  
  switch (c) {
    case 'h': case '?':
      printHelp(); break;
    case 'i':
      incrementInputGain(INCREMENT_INPUTGAIN_DB);
      break;
    case 'I':  
      incrementInputGain(-INCREMENT_INPUTGAIN_DB);
      break;
    case 'k':
      incrementKnobGain(INCREMENT_HEADPHONE_GAIN_DB);
      break;
    case 'K':   
      incrementKnobGain(-INCREMENT_HEADPHONE_GAIN_DB);
      break;
    case 'w':
      myTympan.println("Received: Listen to PCB mics");
      setInputSource(INPUT_PCBMICS);
      setInputMixer(ALL_MICS, 0.5);
      break;
    case 'd':
      myTympan.println("Received: Listen to PDM mics"); //digital mics
      setInputSource(INPUT_PDMMICS);
      setInputMixer(ALL_MICS, 0.5);
      break;
    case 'W':
      myTympan.println("Received: Listen to Mic jack as mic");
      setInputSource(INPUT_MICJACK_MIC);
      setInputMixer(ALL_MICS, 0.5);
      break;
    case 'e':
      myTympan.println("Received: Listen to Mic jack as line-in");
      setInputSource(INPUT_MICJACK_LINEIN);
      setInputMixer(ALL_MICS, 0.5);
      break;
    case 'r':
      myTympan.println("Received: begin SD recording");
      audioSDWriter.startRecording();
      break;
    case 's':
      myTympan.println("Received: stop SD recording");
      audioSDWriter.stopRecording();
      break;
    case '|':
      myTympan.println("Recieved: delete all SD recordings.");
      audioSDWriter.deleteAllRecordings();
      myTympan.println("Delete all SD recordings complete.");
      break;
    case '!':
      myTympan.println("Received: DISABLE Left-Front earpiece mic");
      setInputMixer(MIC_FRONT_LEFT, 0.0);
      break;
    case '1':
      myTympan.println("Received: ENABLE Left-Front earpiece mic");
      setInputMixer(MIC_FRONT_LEFT, 1.0);
      break;
    case '@':
      myTympan.println("Received: DISABLE Left-Rear earpiece mic");
      setInputMixer(MIC_REAR_LEFT, 0.0);
      break;
    case '2':
      myTympan.println("Received: ENABLE Left-Rear earpiece mic");
      setInputMixer(MIC_REAR_LEFT, 1.0);
      break;
    case '#':
      myTympan.println("Received: DISABLE Right-Front earpiece mic");
      setInputMixer(MIC_FRONT_RIGHT, 0.0);
      break;
    case '3':
      myTympan.println("Received: Use Right-Frontearpiece mic");
      setInputMixer(MIC_FRONT_RIGHT, 1.0);
      break;
    case '$':
      myTympan.println("Received: DISABLE Right-Rear earpiece mic");
      setInputMixer(MIC_REAR_RIGHT, 0.0);
      break;
    case '4':
      myTympan.println("Received: Use Right-Rear earpiece mic");
      setInputMixer(MIC_REAR_RIGHT, 1.0);
      break;
    case '%':
      myTympan.println("Received: DISABLE All earpiece mics");
      setInputMixer(ALL_MICS, 0.0);
      break;
    case '5':
      myTympan.println("Received: ENABLE All earpiece mics");
      setInputMixer(ALL_MICS, 1.0);
      break;
  }
}


#endif
