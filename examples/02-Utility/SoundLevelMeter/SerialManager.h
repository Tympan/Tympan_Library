
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>

extern Tympan myTympan;

//now, define the Serial Manager class
class SerialManager {
  public:
    SerialManager(void) {};
    void respondToByte(char c);
    void printHelp(void);

  private:

};

void SerialManager::printHelp(void) {
  myTympan.println();
  myTympan.println("SerialManager Help: Available Commands:");
  myTympan.println("   h: Print this help");
  myTympan.println("   c,C: Enable/Disable printing of CPU and Memory usage");
  //myTympan.println("   F: Self-Generated Test: Frequency sweep.  End-to-End Measurement.");
  myTympan.println("   l,L: Enable/Disable printing of loudness level");
  myTympan.println();
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
      myTympan.println("Command Received: enable printing of memory and CPU usage.");
      enablePrintMemoryAndCPU(true);
      break;
    case 'C':
      myTympan.println("Command Received: disable printing of memory and CPU usage.");
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
    //      myTympan.println("Command Received: starting test using frequency sweep.  Filterbank assessment...");
    //      freqSweepTester_filterbank.begin();
    //      while (!freqSweepTester_filterbank.available()) {delay(100);};
    //      myTympan.println("Press 'h' for help...");
    //      break;
    case 'l':
      myTympan.println("Command Received: enable printing of loudness levels.");
      enablePrintLoudnessLevels(true);
      break;
    case 'L':
      myTympan.println("Command Received: disable printing of loudness levels.");
      enablePrintLoudnessLevels(false);
      break;

  }
};


#endif
