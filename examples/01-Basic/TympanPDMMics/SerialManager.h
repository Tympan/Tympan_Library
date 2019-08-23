    
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>

//add in the algorithm whose gains we wish to set via this SerialManager...change this if your gain algorithms class changes names!
//include "AudioEffectCompWDRC_F32.h"    //change this if you change the name of the algorithm's source code filename
//typedef AudioEffectCompWDRC_F32 GainAlgorithm_t; //change this if you change the algorithm's class name

//now, define the Serial Manager class
class SerialManager {
  public:
    SerialManager(TympanBase &_audioHardware,AudioMixer4_F32 &mixer)
      : audioHardware(_audioHardware), mixerIn(mixer)
        {  };
      
    void respondToByte(char c);
    void printHelp(void); 
    void printGainSettings(void);
    float gainIncrement_dB = 2.5f;  

  private:
    TympanBase &audioHardware;
    AudioMixer4_F32 &mixerIn;

};

void SerialManager::printHelp(void) {
  audioHardware.println();
  audioHardware.println("SerialManager Help: Available Commands:");
  audioHardware.println("   h: Print this help");
  audioHardware.println("   r: begin recording");
  audioHardware.println("   s: stop recording");
  audioHardware.println("   b/n/m/v/d: Switch between (b) on-PCB mic; (n) line-input (single-ended); (m) mic jack; (d) digital PDM mic");
  audioHardware.print  ("   o: Increase the volume by "); audioHardware.print(gainIncrement_dB); audioHardware.println(" dB");
  audioHardware.print  ("   i: Decrease the volume by "); audioHardware.print(gainIncrement_dB); audioHardware.println(" dB");
   audioHardware.println("   C: Toggle printing of CPU and Memory usage");

  audioHardware.println();
}

//Extern Functions
extern void setConfiguration(int config);
extern void togglePrintMemoryAndCPU(void);
//extern void togglePrintAveSignalLevels(bool);
extern void beginRecordingProcess(void);
extern void stopRecording(void);
extern void incrementInputGain(float);

//Extern variables
extern float vol_knob_gain_dB;
extern float input_gain_dB;
extern const int config_pcb_mics;
extern const int config_mic_jack;
extern const int config_line_in_SE;
extern const int config_line_in_diff;
extern const int config_pdm_mic;



//switch yard to determine the desired action
void SerialManager::respondToByte(char c) {
  switch (c) {
    case 'h': case '?':
      printHelp(); break;
    case 'C': case 'c':
      audioHardware.println("Command Received: toggle printing of memory and CPU usage.");
      togglePrintMemoryAndCPU(); break;
    case 'o':
      incrementInputGain(gainIncrement_dB);
      printGainSettings();
      break;
    case 'i':   //which is "shift i"
      incrementInputGain(-gainIncrement_dB);
      printGainSettings();  
      break;
    case 'b':
      audioHardware.println("Command Received: switch to on-PCB mic; InputGain = 0dB");
      setConfiguration(config_pcb_mics);
      break;
    case 'd':
      audioHardware.println("Command Received: switch to digital PDM mics; InputGain = 0dB");
      setConfiguration(config_pdm_mic);
      break;
    case 'n':
      audioHardware.println("Command Received: switch to line-input through-holes (Single-Ended); InputGain = 0dB");
      setConfiguration(config_line_in_SE);
      break;
    case 'm':
      audioHardware.println("Command Received: switch to external mic; InputGain = 0dB");
      setConfiguration(config_mic_jack);
      break;
    case 'v':
      audioHardware.println("Command Received: switch to line-input through holes (DIFF); InputGain = 0dB");
      setConfiguration(config_line_in_diff);
      break;
    
    case 'r':
      beginRecordingProcess();
      break;
    case 's':
       stopRecording();
    break;
  }
}

void SerialManager::printGainSettings(void) {
  audioHardware.print("Vol Knob = "); 
  audioHardware.print(vol_knob_gain_dB,1);
  audioHardware.print(", Input PGA = "); 
  audioHardware.print(input_gain_dB,1);
  audioHardware.println();
}

#endif
