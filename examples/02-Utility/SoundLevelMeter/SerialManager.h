
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>

//now, define the Serial Manager class
class SerialManager {
  public:
    SerialManager(
      TympanBase &_audioHardware //,
      //AudioControlTestFreqSweep_F32 &_freqSweepTester,
    )
      : audioHardware(_audioHardware)//,
        //freqSweepTester(_freqSweepTester),
    { };

    void respondToByte(char c);
    void printHelp(void);

  private:
    TympanBase &audioHardware;
    //AudioControlTestFreqSweep_F32 &freqSweepTester;
};

void SerialManager::printHelp(void) {
  audioHardware.println();
  audioHardware.println("SerialManager Help: Available Commands:");
  audioHardware.println("   h: Print this help");
  audioHardware.println("   c,C: Enable/Disable printing of CPU and Memory usage");
  //audioHardware.println("   F: Self-Generated Test: Frequency sweep.  End-to-End Measurement.");
  audioHardware.println("   l,L: Enable/Disable printing of loudness level");
  audioHardware.println();
}

//functions in the main sketch that I want to call from here
extern void enablePrintMemoryAndCPU(bool);
extern void enablePrintLoudnessLevels(bool);


//switch yard to determine the desired action
void SerialManager::respondToByte(char c) {
  //float old_val = 0.0, new_val = 0.0;
  switch (c) {
    case 'h': case '?':
      printHelp(); break;
    case 'c':
      audioHardware.println("Command Received: enable printing of memory and CPU usage.");
      enablePrintMemoryAndCPU(true);
      break;
    case 'C':
      audioHardware.println("Command Received: disable printing of memory and CPU usage.");
      enablePrintMemoryAndCPU(false);
      break;
    //    case 'f':
    //      //frequency sweep test
    //      { //limit the scope of any variables that I create here
    //        freqSweepTester_filterbank.setSignalAmplitude_dBFS(-30.f);
    //        float start_freq_Hz = 125.0f, end_freq_Hz = 16000.f, step_octave = powf(2.0,1.0/3.0); //pow(2.0,1.0/3.0) is 3 steps per octave
    //        freqSweepTester_filterbank.setStepPattern(start_freq_Hz, end_freq_Hz, step_octave);
    //        freqSweepTester_filterbank.setTargetDurPerStep_sec(0.5);
    //      }
    //      audioHardware.println("Command Received: starting test using frequency sweep.  Filterbank assessment...");
    //      freqSweepTester_filterbank.begin();
    //      while (!freqSweepTester_filterbank.available()) {delay(100);};
    //      audioHardware.println("Press 'h' for help...");
    //      break;
    case 'l':
      audioHardware.println("Command Received: enable printing of loudness levels.");
      enablePrintLoudnessLevels(true);
      break;
    case 'L':
      audioHardware.println("Command Received: disable printing of loudness levels.");
      enablePrintLoudnessLevels(false);
      break;

  }
};


#endif

