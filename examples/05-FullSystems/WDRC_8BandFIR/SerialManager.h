
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>

extern Tympan myTympan;               //defined in main *.ino file
extern bool enable_printCPUandMemory; //defined in main *.ino file

//functions in the main sketch that I want to call from here
extern void incrementKnobGain(float);
extern void printGainSettings(void);
extern void togglePrintAveSignalLevels(bool);
extern void incrementDSLConfiguration(void);

//add in the algorithm whose gains we wish to set via this SerialManager...change this if your gain algorithms class changes names!
#include "AudioEffectCompWDRC_F32.h"    //change this if you change the name of the algorithm's source code filename
typedef AudioEffectCompWDRC_F32 GainAlgorithm_t; //change this if you change the algorithm's class name

//now, define the Serial Manager class
class SerialManager {
  public:
    SerialManager(int n,
          GainAlgorithm_t *gain_algs, 
          AudioControlTestAmpSweep_F32 &_ampSweepTester,
          AudioControlTestFreqSweep_F32 &_freqSweepTester,
          AudioControlTestFreqSweep_F32 &_freqSweepTester_filterbank)
      : gain_algorithms(gain_algs), 
        ampSweepTester(_ampSweepTester), 
        freqSweepTester(_freqSweepTester),
        freqSweepTester_filterbank(_freqSweepTester_filterbank)
        {
          N_CHAN = n;
        };
      
    void respondToByte(char c);
    void printHelp(void);
    void incrementChannelGain(int chan, float change_dB);
    void decreaseChannelGain(int chan);
    void set_N_CHAN(int _n_chan) { N_CHAN = _n_chan; };

    float channelGainIncrement_dB = 2.5f;  
    int N_CHAN;
  private:
    GainAlgorithm_t *gain_algorithms;  //point to first element in array of expanders
    AudioControlTestAmpSweep_F32 &ampSweepTester;
    AudioControlTestFreqSweep_F32 &freqSweepTester;
    AudioControlTestFreqSweep_F32 &freqSweepTester_filterbank;
};

#define MAX_CHANS 8
void printChanUpMsg(int N_CHAN) {
  char fooChar[] = "12345678";
  myTympan.print("   ");
  for (int i=0;i<min(MAX_CHANS,N_CHAN);i++) {
    myTympan.print(fooChar[i]); 
    if (i < (N_CHAN-1)) myTympan.print(",");
  }
  myTympan.print(": Increase linear gain of given channel (1-");
  myTympan.print(N_CHAN);
  myTympan.print(") by ");
}
void printChanDownMsg(int N_CHAN) {
  char fooChar[] = "!@#$%^&*";
  myTympan.print("   ");
  for (int i=0;i<min(MAX_CHANS,N_CHAN);i++) {
    myTympan.print(fooChar[i]); 
    if (i < (N_CHAN-1)) myTympan.print(",");
  }
  myTympan.print(": Decrease linear gain of given channel (1-");
  myTympan.print(N_CHAN);
  myTympan.print(") by ");
}
void SerialManager::printHelp(void) {  
  myTympan.println();
  myTympan.println("SerialManager Help: Available Commands:");
  myTympan.println("   h: Print this help");
  myTympan.println("   g: Print the gain settings of the device.");
  myTympan.println("   c/C: Enable/Disable printing of CPU and Memory usage");
  myTympan.println("   l: Toggle printing of pre-gain per-channel signal levels (dBFS)");
  myTympan.println("   L: Toggle printing of pre-gain per-channel signal levels (dBSPL, per DSL 'maxdB')");
  myTympan.println("   A: Self-Generated Test: Amplitude sweep.  End-to-End Measurement.");
  myTympan.println("   F: Self-Generated Test: Frequency sweep.  End-to-End Measurement.");
  myTympan.println("   f: Self-Generated Test: Frequency sweep.  Measure filterbank.");
  myTympan.print("   k: Increase the gain of all channels (ie, knob gain) by "); myTympan.print(channelGainIncrement_dB); myTympan.println(" dB");
  myTympan.print("   K: Decrease the gain of all channels (ie, knob gain) by ");
  printChanUpMsg(N_CHAN);  myTympan.print(channelGainIncrement_dB); myTympan.println(" dB");
  printChanDownMsg(N_CHAN);  myTympan.print(channelGainIncrement_dB); myTympan.println(" dB");
  myTympan.println("   D: Toggle between DSL configurations: NORMAL vs FULL-ON");
 myTympan.println();
}


//switch yard to determine the desired action
void SerialManager::respondToByte(char c) {
  float old_val = 0.0, new_val = 0.0;
  switch (c) {
    case 'h': case '?':
      printHelp(); break;
    case 'g': case 'G':
      printGainSettings(); break;
    case 'k':
      incrementKnobGain(channelGainIncrement_dB); break;
    case 'K':   //which is "shift k"
      incrementKnobGain(-channelGainIncrement_dB);  break;
    case '1':
      incrementChannelGain(1-1, channelGainIncrement_dB); break;
    case '2':
      incrementChannelGain(2-1, channelGainIncrement_dB); break;
    case '3':
      incrementChannelGain(3-1, channelGainIncrement_dB); break;
    case '4':
      incrementChannelGain(4-1, channelGainIncrement_dB); break;
    case '5':
      incrementChannelGain(5-1, channelGainIncrement_dB); break;
    case '6':
      incrementChannelGain(6-1, channelGainIncrement_dB); break;
    case '7':
      incrementChannelGain(7-1, channelGainIncrement_dB); break;
    case '8':      
      incrementChannelGain(8-1, channelGainIncrement_dB); break;    
    case '!':  //which is "shift 1"
      incrementChannelGain(1-1, -channelGainIncrement_dB); break;
    case '@':  //which is "shift 2"
      incrementChannelGain(2-1, -channelGainIncrement_dB); break;
    case '#':  //which is "shift 3"
      incrementChannelGain(3-1, -channelGainIncrement_dB); break;
    case '$':  //which is "shift 4"
      incrementChannelGain(4-1, -channelGainIncrement_dB); break;
    case '%':  //which is "shift 5"
      incrementChannelGain(5-1, -channelGainIncrement_dB); break;
    case '^':  //which is "shift 6"
      incrementChannelGain(6-1, -channelGainIncrement_dB); break;
    case '&':  //which is "shift 7"
      incrementChannelGain(7-1, -channelGainIncrement_dB); break;
    case '*':  //which is "shift 8"
      incrementChannelGain(8-1, -channelGainIncrement_dB); break;          
    case 'A':
      //amplitude sweep test
      { //limit the scope of any variables that I create here
        ampSweepTester.setSignalFrequency_Hz(1000.f);
        float start_amp_dB = -100.0f, end_amp_dB = 0.0f, step_amp_dB = 5.0f;
        ampSweepTester.setStepPattern(start_amp_dB, end_amp_dB, step_amp_dB);
        ampSweepTester.setTargetDurPerStep_sec(1.0);
      }
      myTympan.println("Command Received: starting test using amplitude sweep...");
      ampSweepTester.begin();
      while (!ampSweepTester.available()) {delay(100);};
      myTympan.println("Press 'h' for help...");
      break;
    case 'c':      
      myTympan.println("Received: printing memory and CPU usage.");
      enable_printCPUandMemory = true;
      break;     
    case 'C':
      myTympan.println("Received: stopping printing memory and CPU usage.");
      enable_printCPUandMemory = false;
      break;     
    case 'D':
      myTympan.println("Command Received: changing DSL configuration...you will lose any custom gain values...");
      incrementDSLConfiguration();
      break;
    case 'F':
      //frequency sweep test...end-to-end
      { //limit the scope of any variables that I create here
        freqSweepTester.setSignalAmplitude_dBFS(-70.f);
        float start_freq_Hz = 125.0f, end_freq_Hz = 12000.f, step_octave = powf(2.0,1.0/6.0); //pow(2.0,1.0/3.0) is 3 steps per octave
        freqSweepTester.setStepPattern(start_freq_Hz, end_freq_Hz, step_octave);
        freqSweepTester.setTargetDurPerStep_sec(1.0);
      }
      myTympan.println("Command Received: starting test using frequency sweep, end-to-end assessment...");
      freqSweepTester.begin();
      while (!freqSweepTester.available()) {delay(100);};
      myTympan.println("Press 'h' for help...");
      break; 
    case 'f':
      //frequency sweep test
      { //limit the scope of any variables that I create here
        freqSweepTester_filterbank.setSignalAmplitude_dBFS(-30.f);
        float start_freq_Hz = 125.0f, end_freq_Hz = 12000.f, step_octave = powf(2.0,1.0/6.0); //pow(2.0,1.0/3.0) is 3 steps per octave
        freqSweepTester_filterbank.setStepPattern(start_freq_Hz, end_freq_Hz, step_octave);
        freqSweepTester_filterbank.setTargetDurPerStep_sec(0.5);
      }
      myTympan.println("Command Received: starting test using frequency sweep.  Filterbank assessment...");
      freqSweepTester_filterbank.begin();
      while (!freqSweepTester_filterbank.available()) {delay(100);};
      myTympan.println("Press 'h' for help...");
      break;      
    case 'l':
      myTympan.println("Command Received: toggle printing of per-band ave signal levels.");
      { bool as_dBSPL = false; togglePrintAveSignalLevels(as_dBSPL); }
      break;
    case 'L':
      myTympan.println("Command Received: toggle printing of per-band ave signal levels.");
      { bool as_dBSPL = true; togglePrintAveSignalLevels(as_dBSPL); }
      break; 
    case 'u':
    {
      old_val = myTympan.getHPCutoff_Hz(); new_val = min(old_val*sqrt(2.0), 8000.0); //half-octave steps up
      float fs_Hz = myTympan.getSampleRate_Hz();
      myTympan.setHPFonADC(true,new_val,fs_Hz);
      myTympan.print("Command received: Increasing ADC HP Cutoff to "); myTympan.print(myTympan.getHPCutoff_Hz());myTympan.println(" Hz");
    }
      break;
    case 'U':
    {
      old_val = myTympan.getHPCutoff_Hz(); new_val = max(old_val/sqrt(2.0), 5.0); //half-octave steps down
      float fs_Hz = myTympan.getSampleRate_Hz();
      myTympan.setHPFonADC(true,new_val,fs_Hz);
      myTympan.print("Command received: Decreasing ADC HP Cutoff to "); myTympan.print(myTympan.getHPCutoff_Hz());myTympan.println(" Hz");   
      break;
    }

  }
}

void SerialManager::incrementChannelGain(int chan, float change_dB) {
  if (chan < N_CHAN) {
    gain_algorithms[chan].incrementGain_dB(change_dB);
    //myTympan.print("Incrementing gain on channel ");myTympan.print(chan);
    //myTympan.print(" by "); myTympan.print(change_dB); myTympan.println(" dB");
    printGainSettings();  //in main sketch file
  }
}

#endif
