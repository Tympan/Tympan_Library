/* 
 *  SerialManager
 *  
 *  Created: Chip Audette, OpenAudio, Nov 2021
 *  
 *  Purpose: This helps you process commands coming in over the serial link.
 *  
 */


#ifndef _SerialManager_h
#define _SerialManager_h


//functions in the main sketch that I want to call from here
extern float incrementChannelGain(float,int i);
extern void printEqSettings(void);
extern int n_eq_bands;

//now, define the Serial Manager class
class SerialManager {
  public:
    SerialManager(void) {};
      
    void respondToByte(char c);
    void printHelp(void);
    float gainIncrement_dB = 1.0f;  
  private:

};

void SerialManager::printHelp(void) {
  Serial.println();
  Serial.println(String(n_eq_bands) + "-Band Equalizer, Help: Available Commands:");
  Serial.println("   h: Print this help");
  Serial.println("   g: Print the gain for each EQ band");
  Serial.println("   1,2,3,4,5,6,7,8: Increase gain for given EQ band");
  Serial.println("   q,w,e,r,t,y,u,i: Decrease gain for given EQ band");
  Serial.println();
}

//switch yard to determine the desired action
void SerialManager::respondToByte(char c) {
  float val=0;
  switch (c) {
    case 'h': case '?':
      printHelp(); break;
    case 'g': case 'G':
      printEqSettings(); break;
    case '1':
      val=incrementChannelGain(gainIncrement_dB,0); Serial.println("Changed gain 1 to " + String(val) + " dB"); break;
    case '2':
      val=incrementChannelGain(gainIncrement_dB,1); Serial.println("Changed gain 2 to " + String(val) + " dB");  break;
    case '3':
      val=incrementChannelGain(gainIncrement_dB,2); Serial.println("Changed gain 3 to " + String(val) + " dB");  break;
    case '4':
      val=incrementChannelGain(gainIncrement_dB,3); Serial.println("Changed gain 4 to " + String(val) + " dB");  break;
    case '5':
      val=incrementChannelGain(gainIncrement_dB,4); Serial.println("Changed gain 5 to " + String(val) + " dB");  break;
    case '6':
      val=incrementChannelGain(gainIncrement_dB,5); Serial.println("Changed gain 6 to " + String(val) + " dB");  break;
    case '7':
      val=incrementChannelGain(gainIncrement_dB,6); Serial.println("Changed gain 7 to " + String(val) + " dB");  break;
    case '8':
      val=incrementChannelGain(gainIncrement_dB,7); Serial.println("Changed gain 8 to " + String(val) + " dB");  break;
    case 'q':
      val=incrementChannelGain(-gainIncrement_dB,0); Serial.println("Changed gain 1 to " + String(val) + " dB");  break;
    case 'w':
      val=incrementChannelGain(-gainIncrement_dB,1); Serial.println("Changed gain 2 to " + String(val) + " dB");  break;
    case 'e':
      val=incrementChannelGain(-gainIncrement_dB,2); Serial.println("Changed gain 3 to " + String(val) + " dB");  break;
    case 'r':
      val=incrementChannelGain(-gainIncrement_dB,3); Serial.println("Changed gain 4 to " + String(val) + " dB");  break;
    case 't':
      val=incrementChannelGain(-gainIncrement_dB,4); Serial.println("Changed gain 5 to " + String(val) + " dB");  break;
    case 'y':
      val=incrementChannelGain(-gainIncrement_dB,5); Serial.println("Changed gain 6 to " + String(val) + " dB");  break;
    case 'u':
      val=incrementChannelGain(-gainIncrement_dB,6); Serial.println("Changed gain 7 to " + String(val) + " dB");  break;
    case 'i':
      val=incrementChannelGain(-gainIncrement_dB,7); Serial.println("Changed gain 8 to " + String(val) + " dB");  break;
  }
}


#endif
