#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>

//Extern variables...originally defined in the main *.ino file
extern Tympan myTympan;
extern AudioSDWriter_F32 audioSDWriter;

//Extern Functions
extern void setInputSource(int);
extern void setInputMixer(int, float);
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
  Serial.println();
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println("  h  : Print this help");
  Serial.println("  d  : Inputs: Use PDM Mics from Tympan Earpieces as input");
  Serial.println("  w  : Inputs: Use PCB Mics");
  Serial.println("  W  : Inputs: Use Mic on Mic Jack");
  Serial.println("  e  : Inputs: Use LineIn on Mic Jack");
  Serial.println("  i/I: Gain: Increase/Decrease Input by " + String(INCREMENT_INPUTGAIN_DB) + " dB");
  Serial.println("  k/K: Gain: Increase/Decrease Headphone Volume by " + String(INCREMENT_HEADPHONE_GAIN_DB) + " dB");
  Serial.println("  r,s,|: SD: begin/stop/deleteAll recording");
  Serial.println();
  Serial.println("  1  : Mixer: Switch to Left-Front Earpiece Mic (L)");
  Serial.println("  2  : Mixer: Switch to Left-Rear Earpiece Mic (L)");
  Serial.println("  3  : Mixer: Switch to Right-Front Earpiece Mic (R)");
  Serial.println("  4  : Mixer: Switch to Right-Rear Earpiece Mic (R)");
  Serial.println("  5  : Mixer: Switch to Both Front Mics (L,R)");
  Serial.println("  6  : Mixer: Switch to Both Rear Mics (L,R)");
  Serial.println("  7  : Mixer: Switch to Tympan AIC only (L,R)");
  Serial.println("  8  : Mixer: Switch to Earpiece Shield AIC only (L,R)");
  Serial.println("  9  : Mixer: Switch to Mixing Front+Rear Mics (L,R)");
  Serial.println();
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
      Serial.println("Received: Listen to PCB mics");
      setInputSource(INPUT_PCBMICS);
      setInputMixer(MIC_AIC0, 1.0);
      break;
    case 'd':
      Serial.println("Received: Listen to PDM mics"); //digital mics
      setInputSource(INPUT_PDMMICS);
      setInputMixer(MIC_FRONT_BOTH, 1.0);
      break;
    case 'W':
      Serial.println("Received: Listen to Mic jack as mic");
      setInputSource(INPUT_MICJACK_MIC);
      setInputMixer(MIC_AIC0, 1.0);
      break;
    case 'e':
      Serial.println("Received: Listen to Mic jack as line-in");
      setInputSource(INPUT_MICJACK_LINEIN);
      setInputMixer(MIC_AIC0, 1.0);
      break;
    case 'r':
      Serial.println("Received: begin SD recording");
      audioSDWriter.startRecording();
      break;
    case 's':
      Serial.println("Received: stop SD recording");
      audioSDWriter.stopRecording();
      break;
    case '|':
      Serial.println("Recieved: delete all SD recordings.");
      audioSDWriter.deleteAllRecordings();
      Serial.println("Delete all SD recordings complete.");
      break;
    case '1':
      Serial.println("Received: Use Left-Front earpiece mic");
      setInputMixer(MIC_FRONT_LEFT, 1.0);
      break;
    case '2':
      Serial.println("Received: Use Left-Rear earpiece mic");
      setInputMixer(MIC_REAR_LEFT, 1.0);
      break;
    case '3':
      Serial.println("Received: Use Right-Front earpiece mic");
      setInputMixer(MIC_FRONT_RIGHT, 1.0);
      break;
    case '4':
      Serial.println("Received: Use Right-Rear earpiece mic");
      setInputMixer(MIC_REAR_RIGHT, 1.0);
      break;
    case '5':
      Serial.println("Received: Switch to both front mics");
      setInputMixer(MIC_FRONT_BOTH, 1.0);
      break;
    case '6':
      Serial.println("Received: Switch to both rear mics");
      setInputMixer(MIC_REAR_BOTH, 1.0);
      break;
    case '7':
      Serial.println("Received: Switch to only AIC0 (Main Tympan)");
      setInputMixer(MIC_AIC0, 1.0);
      break;
    case '8':
      Serial.println("Received: Switch to only AIC1 (Main Tympan)");
      setInputMixer(MIC_AIC1, 1.0);
      break;      
    case '9':
      Serial.println("Received: Mix front+back mics");
      setInputMixer(ALL_MICS, 0.5);
      break;
    case '0':
      Serial.println("Received: Mute all earpiece mics");
      setInputMixer(ALL_MICS, 0.0);
      break;      
  }
}


#endif
