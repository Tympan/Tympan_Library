
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>
extern Tympan myTympan;               
extern AudioFeedbackCancelNLMS_F32 afc;

extern float incrementDigitalGain(float);
extern void printGainSettings(void);
extern bool enable_printCPUandMemory;
extern float incrementKnobGain(float);

class SerialManager : public SerialManagerBase  {  // see Tympan_Library SerialManagerBase for more functions!
  public:
    void printHelp(void);
    bool processCharacter(char c);  //this is called by SerialManagerBase.respondToByte(char c)
    
    float gainIncrement_dB = 1.0f; 
};


void SerialManager::printHelp(void) {  
  Serial.println();
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println("   h: Print this help");
  Serial.println("   g: Print the gain settings of the device.");
  Serial.println("   c/C: Enable/Disable printing of CPU and Memory usage");
  Serial.println("   k/K: Incr/Decrease overall gain by " + String(gainIncrement_dB) + " dB");
  Serial.println("   a,A: Enable or Disable Adaptive Feedback Cancelation (currently " + String(afc.getEnable()) + ")");
  Serial.println("   m,M: Increase or Decrease AFC mu (currently " + String(afc.getMu(),6) + ")");
  Serial.println("   r,R: Increase or Decrease AFC rho (currently  " + String(afc.getRho(),6) + ")");
  Serial.println("   e,E: Increase or Decrease AFC eps (currently " + String(afc.getEps(),6) + ")");
  Serial.println("   x,X: Increase or Decrease AFC filter length (currently " + String(afc.getAfl()) + ")");
  Serial.println("   i: Print settings of the current AFC algorithm");
  Serial.println("   z: Re-initialize the AFC states");
  Serial.println();
}


//switch yard to determine the desired action
bool SerialManager::processCharacter(char c) {
  bool ret_val = true;
  float old_val = 0.0, new_val = 0.0;
  switch (c) {
    case 'h': case '?':
      printHelp(); break;
    case 'g': case 'G':
      printGainSettings(); break;
    case 'c':      
      Serial.println("Received: printing memory and CPU usage.");
      enable_printCPUandMemory = true;
      break;     
    case 'C':
      Serial.println("Received: stopping printing memory and CPU usage.");
      enable_printCPUandMemory = false;
      break; 
    case 'k':
      incrementKnobGain(gainIncrement_dB); break;
    case 'K':
      incrementKnobGain(-gainIncrement_dB);  break;
    case 'a':
      Serial.println("Command Received: enabling adaptive feedback cancelation.");
      afc.setEnable(true);
      break;
    case 'A':
      Serial.println("Command Received: disabling adaptive feedback cancelation.");      
      afc.setEnable(false);
      break;
    case 'm':
      old_val = afc.getMu(); new_val = old_val * 2.0;
      Serial.print("Command received: increasing AFC mu to "); Serial.println(afc.setMu(new_val),6);
      break;
    case 'M':
      old_val = afc.getMu(); new_val = old_val / 2.0;
      Serial.print("Command received: decreasing AFC mu to "); Serial.println(afc.setMu(new_val),6);
      break;
    case 'r':
      old_val = afc.getRho(); new_val = 1.0-((1.0-old_val)/sqrt(2.0));
      Serial.print("Command received: increasing AFC rho to "); Serial.println(afc.setRho(new_val),6);
      break;
    case 'R':
      old_val = afc.getRho(); new_val = 1.0-((1.0-old_val)*sqrt(2.0));
      Serial.print("Command received: increasing AFC rho to "); Serial.println(afc.setRho(new_val),6);
      break;
    case 'e':
      old_val = afc.getEps(); new_val = old_val*sqrt(10.0);
      Serial.print("Command received: increasing AFC eps to "); Serial.println(afc.setEps(new_val),6);
      break;
    case 'E':
      old_val = afc.getEps(); new_val = old_val/sqrt(10.0);
      Serial.print("Command received: increasing AFC eps to "); Serial.println(afc.setEps(new_val),6);
      break;    
    case 'x':
      old_val = afc.getAfl(); new_val = old_val + 5;
      Serial.print("Command received: increasing AFC filter length to "); Serial.println(afc.setAfl(new_val));
      break;    
    case 'X':
      old_val = afc.getAfl(); new_val = old_val - 5;
      Serial.print("Command received: decreasing AFC filter length to "); Serial.println(afc.setAfl(new_val));
      break;  
    case 'i':
      afc.printAlgorithmInfo();
      break;
	case 'z':
      Serial.print("Command received: re-initializing the AFC states...");
      afc.setEnable(false);
      afc.initializeStates();
      afc.setEnable(true);
      break;
    default:
      ret_val = SerialManagerBase::processCharacter(c);  //in here, it automatically loops over the different UI elements
      break; 
  }

  return ret_val;
}
#endif
