
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

//now, define the Serial Manager class
class SerialManager {
  public:
    SerialManager(Stream *_s) { s = _s; };
      
    void respondToByte(char c);
    void printHelp(void);
    float gainIncrement_dB = 2.5f;  
    float freqIncrementFactor = sqrt(2.0); //move half an octave with each step
  private:
    Stream *s = NULL;

};

void SerialManager::printHelp(void) {
  s->println();
  s->println("SerialManager Help: Available Commands:");
  s->println("   h: Print this help");
  s->println("   g: Print the gain settings of the device.");
  s->println("   C: Toggle printing of CPU and Memory usage");
  s->print("   k: Increase the volume by "); s->print(gainIncrement_dB); s->println(" dB");
  s->print("   K: Decrease the volume by "); s->print(gainIncrement_dB); s->println(" dB");
  s->print("   f: Raise the highpass filter cutoff frequency by "); s->print((freqIncrementFactor-1.0)*100.0,0); s->println("%");
  s->print("   F: Lower the highpass filter cutoff frequency by "); s->print((freqIncrementFactor-1.0)*100.0,0); s->println("%");
  s->println();
}

//functions in the main sketch that I want to call from here
extern void incrementKnobGain(float);
extern void incrementHPCutoffFreq_Hz(float);
extern void printGainSettings(void);
extern void togglePrintMemoryAndCPU(void);

//switch yard to determine the desired action
void SerialManager::respondToByte(char c) {
  switch (c) {
    case 'h': case '?':
      printHelp(); break;
    case 'g': case 'G':
      printGainSettings(); break;
    case 'C': case 'c':
      s->println("Command Received: toggle printing of memory and CPU usage.");
      togglePrintMemoryAndCPU(); break;     
    case 'k':
      incrementKnobGain(gainIncrement_dB); break;
    case 'K':   //which is "shift k"
      incrementKnobGain(-gainIncrement_dB);  break;
    case 'f':
      incrementHPCutoffFreq_Hz(freqIncrementFactor); break;
    case 'F':
      incrementHPCutoffFreq_Hz(1.0/freqIncrementFactor); break;
  }
}


#endif
