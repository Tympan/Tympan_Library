    
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>


//Extern Functions
extern void setConfiguration(int config);
extern void togglePrintMemoryAndCPU(void);
extern void setPrintMemoryAndCPU(bool);
extern void incrementInputGain(float);
extern void incrementHeadphoneVol(float);


//Extern variables
extern Tympan myTympan;
extern AudioSDWriter_F32 audioSDWriter;
extern float vol_knob_gain_dB;
extern float input_gain_dB;
extern const int config_pcb_mics;
extern const int config_mic_jack;
extern const int config_line_in_SE;
extern const int config_line_in_diff;
extern const int config_pdm_mic;

//now, define the Serial Manager class
class SerialManager {
  public:
    SerialManager(AudioMixer4_F32 &mixer)
      : mixerIn(mixer)
        {  };
      
    void respondToByte(char c);
    void printHelp(void); 
    void printGainSettings(void);
    const float INCREMENT_INPUT_GAIN_DB = 2.5f;  
    const float INCREMENT_HEADPHONE_GAIN_DB = 2.5f;  

  private:
    AudioMixer4_F32 &mixerIn;

};

void SerialManager::printHelp(void) {
  myTympan.println();
  myTympan.println("SerialManager Help: Available Commands:");
  myTympan.println("   h: Print this help");
  myTympan.println("   c: Start printing of CPU and Memory usage");
  myTympan.println("   C: Stop printing of CPU and Memory usage");  
  myTympan.println("   r: begin recording");
  myTympan.println("   s: stop recording");
  myTympan.println("   b/n/m/v/d: Switch between (b) on-PCB mic; (n) line-input (single-ended); (m) mic jack; (d) digital PDM mic");
  myTympan.print  ("   i/o: Decrease/Increase the Input Gain by "); myTympan.print(INCREMENT_INPUT_GAIN_DB); myTympan.println(" dB");
  myTympan.print  ("   j/k: Decrease/Increase the Headphone Volume by "); myTympan.print(INCREMENT_HEADPHONE_GAIN_DB); myTympan.println(" dB");


  myTympan.println();
}



//switch yard to determine the desired action
void SerialManager::respondToByte(char c) {
  switch (c) {
    case 'h': case '?':
      printHelp(); break;
    case 'c':
      myTympan.println("Received: start CPU reporting");
      setPrintMemoryAndCPU(true);
      //setButtonState("cpuStart",true);
      break;
    case 'C':
      myTympan.println("Received: stop CPU reporting");
      setPrintMemoryAndCPU(false);
      //setButtonState("cpuStart",false);
      break;
    case 'o':
      incrementInputGain(INCREMENT_INPUT_GAIN_DB);
      printGainSettings();
      break;
    case 'i':   //which is "shift i"
      incrementInputGain(-INCREMENT_INPUT_GAIN_DB);
      printGainSettings();  
      break;
    case 'k':
      incrementHeadphoneVol(INCREMENT_HEADPHONE_GAIN_DB);
      printGainSettings();
      break;
    case 'j':   //which is "shift i"
      incrementHeadphoneVol(-INCREMENT_HEADPHONE_GAIN_DB);
      printGainSettings();  
      break;
    case 'b':
      myTympan.println("Command Received: switch to on-PCB mic; InputGain = 0dB");
      setConfiguration(config_pcb_mics);
      break;
    case 'd':
      myTympan.println("Command Received: switch to digital PDM mics; InputGain = 0dB");
      setConfiguration(config_pdm_mic);
      break;
    case 'n':
      myTympan.println("Command Received: switch to line-input through-holes (Single-Ended); InputGain = 0dB");
      setConfiguration(config_line_in_SE);
      break;
    case 'm':
      myTympan.println("Command Received: switch to external mic; InputGain = 0dB");
      setConfiguration(config_mic_jack);
      break;
    case 'v':
      myTympan.println("Command Received: switch to line-input through holes (DIFF); InputGain = 0dB");
      setConfiguration(config_line_in_diff);
      break;
   
    case 'r':
      myTympan.println("Received: begin SD recording");
      //beginRecordingProcess();
      audioSDWriter.startRecording();
      if (audioSDWriter.getState() == AudioSDWriter_F32::STATE::RECORDING) {
        //myTympan.print("SD Filename = "); myTympan.println(audioSDWriter.getCurrentFilename());
      } else {
        myTympan.print("SD Failed to start recording."); 
      }
      
      //setButtonState("recordStart",true);
      break;
    case 's':
      myTympan.println("Received: stop SD recording");
      audioSDWriter.stopRecording();
      //setButtonState("recordStart",false);
      break;
    break;
  }
}

void SerialManager::printGainSettings(void) {
  myTympan.print("Vol Knob = "); 
  myTympan.print(vol_knob_gain_dB,1);
  myTympan.print(", Input PGA = "); 
  myTympan.print(input_gain_dB,1);
  myTympan.println();
}

#endif
