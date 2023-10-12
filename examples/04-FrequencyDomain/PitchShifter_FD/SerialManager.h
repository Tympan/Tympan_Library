

#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>

//class instances from main that I'll call here
extern Tympan myTympan;

//functions in the main sketch that I want to call from here
extern void incrementKnobGain(float);
extern void printGainSettings(void);
extern void togglePrintMemoryAndCPU(void);
extern float incrementPitchShift(float);
extern void switchToPCBMics(void);
extern void switchToMicInOnMicJack(void);
extern void switchToLineInOnMicJack(void);


//now, define the Serial Manager class
class SerialManager {
  public:
    SerialManager(void)  {  };
          
    void respondToByte(char c);
    void printHelp(void);
    float channelGainIncrement_dB = 3.0f; 
    float scale_fac_increment = powf(2.0,+1/12.0); //shift by 1 semitone
    
  private:

};
void SerialManager::printHelp(void) {  
  Serial.println();
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println("   h: Print this help");
  Serial.println("   g: Print the gain settings of the device.");
  Serial.println("   C: Toggle printing of CPU and Memory usage");
  Serial.println("   p: Switch to built-in PCB microphones");
  Serial.println("   m: switch to external mic via mic jack");
  Serial.println("   l: switch to line-in via mic jack");
  Serial.println("   k/K: Incr/Decrease the gain of all channels (ie, knob gain) by " + String(channelGainIncrement_dB,2) + " dB");
  Serial.println("   f/F: Raise/Lower freq shifting (change by " + String(scale_fac_increment,3) + "x)");
}


//switch yard to determine the desired action
void SerialManager::respondToByte(char c) {
  //float old_val = 0.0, new_val = 0.0;
  switch (c) {
    case 'h': case '?':
      printHelp(); break;
    case 'g': case 'G':
      printGainSettings(); break;
    case 'k':
      incrementKnobGain(channelGainIncrement_dB); break;
    case 'K':   //which is "shift k"
      incrementKnobGain(-channelGainIncrement_dB);  break;
    case 'C': case 'c':
      Serial.println("Received: toggle printing of memory and CPU usage.");
      togglePrintMemoryAndCPU(); break;
    case 'p':
      switchToPCBMics(); break;
    case 'm':
      switchToMicInOnMicJack(); break;
    case 'l':
      switchToLineInOnMicJack();break;
    case 'f':
      { float new_val = incrementPitchShift(scale_fac_increment);
      Serial.print("Recieved: new freq shift = "); Serial.println(new_val);}
      break;
    case 'F':
      { float new_val = incrementPitchShift(1.0f/scale_fac_increment);
      Serial.print("Recieved: new freq shift = "); Serial.println(new_val);}
      break;
  }
}


#endif
