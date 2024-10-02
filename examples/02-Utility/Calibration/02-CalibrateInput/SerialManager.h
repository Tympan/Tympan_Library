
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>


//Extern variables from the main *.ino file
extern Tympan myTympan;
extern AudioSDWriter_F32 audioSDWriter;
extern float input_gain_dB;
extern float setInputGain_dB(float val_dB);
extern bool flag_printInputLevelToUSB;

class SerialManager : public SerialManagerBase  {  // see Tympan_Library for SerialManagerBase for more functions!
  public:
    SerialManager(void) : SerialManagerBase() {};

    void printHelp(void);
    bool processCharacter(char c);  //this is called automatically by SerialManagerBase.respondToByte(char c)

    float inputGainIncrement_dB = 5.0;  //changes the input gain of the AIC
  private:
};

void SerialManager::printHelp(void) {  
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println("General: No Prefix");
  Serial.println("  h:     Print this help");
  Serial.println("  i/I:   Input: Increase or decrease input gain (current = " + String(input_gain_dB,1) + " dB)");
  Serial.print(  "  p/P:   Printing: start/Stop printing of input signal levels"); if (flag_printInputLevelToUSB)   {Serial.println(" (active)");} else { Serial.println(" (off)"); }
  Serial.println("  r/s:   SD: Start recording (r) or stop (s) audio to SD card");
  Serial.println();
}


//switch yard to determine the desired action
bool SerialManager::processCharacter(char c) { //this is called by SerialManagerBase.respondToByte(char c)
  bool ret_val = true; //assume at first that we will find a match
  switch (c) {
    case 'h': 
      printHelp(); 
      break;
    case 'i':
      setInputGain_dB(input_gain_dB + inputGainIncrement_dB);
      Serial.println("SerialManager: Increased input gain to: " + String(input_gain_dB) + " dB");
      break;
    case 'I':   //which is "shift i"
      setInputGain_dB(input_gain_dB - inputGainIncrement_dB);
      Serial.println("SerialManager: Decreased input gain to: " + String(input_gain_dB) + " dB");
      break;
    case 'p':
      flag_printInputLevelToUSB = true;
      Serial.println("SerialManager: enabled printing of the input signal levels");
      break;
    case 'P':
      flag_printInputLevelToUSB = false;
      Serial.println("SerialManager: disabled printing of the input signal levels");
      break;
    case 'r':
      Serial.println("SerialManager: starting recording of input sginals to the SD card...");
      audioSDWriter.startRecording();
      break;
    case 's':
      Serial.println("SerialManager: stopping recording of input signals to the SD card...");
      audioSDWriter.stopRecording();
      break;
    default:
      Serial.println("SerialManager: command " + String(c) + " not recognized");
      break;
  }
  return ret_val;
}


#endif
