
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>

//add in the algorithm whose gains we wish to set via this SerialManager...change this if your gain algorithms class changes names!
#include "AudioEffectCompWDRC_F32.h"    //change this if you change the name of the algorithm's source code filename
typedef AudioEffectCompWDRC_F32 GainAlgorithm_t; //change this if you change the algorithm's class name

//now, define the Serial Manager class
class SerialManager {
  public:
    SerialManager(Stream *_s, int n,
          TympanBase &_audioHardware,
          GainAlgorithm_t *gain_algs, 
          AudioControlTestAmpSweep_F32 &_ampSweepTester,
          AudioControlTestFreqSweep_F32 &_freqSweepTester,
          AudioControlTestFreqSweep_F32 &_freqSweepTester_filterbank,
          AudioEffectFeedbackCancel_F32 &_feedbackCancel)
      : audioHardware(_audioHardware),
        gain_algorithms(gain_algs), 
        ampSweepTester(_ampSweepTester), 
        freqSweepTester(_freqSweepTester),
        freqSweepTester_filterbank(_freqSweepTester_filterbank),
        feedbackCanceler(_feedbackCancel)
        {
          s = _s;
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
    Stream *s;
    TympanBase &audioHardware;
    GainAlgorithm_t *gain_algorithms;  //point to first element in array of expanders
    AudioControlTestAmpSweep_F32 &ampSweepTester;
    AudioControlTestFreqSweep_F32 &freqSweepTester;
    AudioControlTestFreqSweep_F32 &freqSweepTester_filterbank;
    AudioEffectFeedbackCancel_F32 &feedbackCanceler;
};

#define MAX_CHANS 8
void printChanUpMsg(Stream *s, int N_CHAN) {
  char fooChar[] = "12345678";
  s->print("   ");
  for (int i=0;i<min(MAX_CHANS,N_CHAN);i++) {
    s->print(fooChar[i]); 
    if (i < (N_CHAN-1)) s->print(",");
  }
  s->print(": Increase linear gain of given channel (1-");
  s->print(N_CHAN);
  s->print(") by ");
}
void printChanDownMsg(Stream *s, int N_CHAN) {
  char fooChar[] = "!@#$%^&*";
  s->print("   ");
  for (int i=0;i<min(MAX_CHANS,N_CHAN);i++) {
    s->print(fooChar[i]); 
    if (i < (N_CHAN-1)) s->print(",");
  }
  s->print(": Decrease linear gain of given channel (1-");
  s->print(N_CHAN);
  s->print(") by ");
}
void SerialManager::printHelp(void) {  
  s->println();
  s->println("SerialManager Help: Available Commands:");
  s->println("   h: Print this help");
  s->println("   g: Print the gain settings of the device.");
  s->println("   C: Toggle printing of CPU and Memory usage");
  s->println("   l: Toggle printing of pre-gain per-channel signal levels (dBFS)");
  s->println("   L: Toggle printing of pre-gain per-channel signal levels (dBSPL, per DSL 'maxdB')");
  s->println("   A: Self-Generated Test: Amplitude sweep.  End-to-End Measurement.");
  s->println("   F: Self-Generated Test: Frequency sweep.  End-to-End Measurement.");
  s->println("   f: Self-Generated Test: Frequency sweep.  Measure filterbank.");
  s->print("   k: Increase the gain of all channels (ie, knob gain) by "); s->print(channelGainIncrement_dB); s->println(" dB");
  s->print("   K: Decrease the gain of all channels (ie, knob gain) by ");
  printChanUpMsg(s,N_CHAN);  s->print(channelGainIncrement_dB); s->println(" dB");
  printChanDownMsg(s,N_CHAN);  s->print(channelGainIncrement_dB); s->println(" dB");
  s->println("   D: Toggle between DSL configurations: NORMAL vs FULL-ON");
  s->println("   p,P: Enable or Disable Adaptive Feedback Cancelation.");
  s->print("   m,M: Increase or Decrease AFC mu (currently "); s->print(feedbackCanceler.getMu(),6) ; s->println(").");
  s->print("   r,R: Increase or Decrease AFC rho (currently "); s->print(feedbackCanceler.getRho(),6) ; s->println(").");
  s->print("   e,E: Increase or Decrease AFC eps (currently "); s->print(feedbackCanceler.getEps(),6) ; s->println(").");
  s->print("   x,X: Increase or Decrease AFC filter length (currently "); s->print(feedbackCanceler.getAfl()) ; s->println(").");
  s->print("   u,U: Increase or Decrease Cutoff Frequency of HP Prefilter (currently "); s->print(audioHardware.getHPCutoff_Hz()); s->println(").");
  //s->print("   z,Z: Increase or Decrease AFC N_Coeff_To_Zero (currently "); s->print(feedbackCanceler.getNCoeffToZero()) ; s->println(" Hz).");  
  s->println();
}

//functions in the main sketch that I want to call from here
extern void incrementKnobGain(float);
extern void printGainSettings(void);
extern void printGainSettings(Stream *);
extern void togglePrintMemoryAndCPU(void);
extern void togglePrintAveSignalLevels(bool);
extern void incrementDSLConfiguration(Stream *);

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
      s->println("Command Received: starting test using amplitude sweep...");
      ampSweepTester.begin();
      while (!ampSweepTester.available()) {delay(100);};
      s->println("Press 'h' for help...");
      break;
    case 'C': case 'c':
      s->println("Command Received: toggle printing of memory and CPU usage.");
      togglePrintMemoryAndCPU(); break;
    case 'D':
      s->println("Command Received: changing DSL configuration...you will lose any custom gain values...");
      incrementDSLConfiguration(s);
      break;
    case 'F':
      //frequency sweep test...end-to-end
      { //limit the scope of any variables that I create here
        freqSweepTester.setSignalAmplitude_dBFS(-70.f);
        float start_freq_Hz = 125.0f, end_freq_Hz = 12000.f, step_octave = powf(2.0,1.0/6.0); //pow(2.0,1.0/3.0) is 3 steps per octave
        freqSweepTester.setStepPattern(start_freq_Hz, end_freq_Hz, step_octave);
        freqSweepTester.setTargetDurPerStep_sec(1.0);
      }
      s->println("Command Received: starting test using frequency sweep, end-to-end assessment...");
      freqSweepTester.begin();
      while (!freqSweepTester.available()) {delay(100);};
      s->println("Press 'h' for help...");
      break; 
    case 'f':
      //frequency sweep test
      { //limit the scope of any variables that I create here
        freqSweepTester_filterbank.setSignalAmplitude_dBFS(-30.f);
        float start_freq_Hz = 125.0f, end_freq_Hz = 12000.f, step_octave = powf(2.0,1.0/6.0); //pow(2.0,1.0/3.0) is 3 steps per octave
        freqSweepTester_filterbank.setStepPattern(start_freq_Hz, end_freq_Hz, step_octave);
        freqSweepTester_filterbank.setTargetDurPerStep_sec(0.5);
      }
      s->println("Command Received: starting test using frequency sweep.  Filterbank assessment...");
      freqSweepTester_filterbank.begin();
      while (!freqSweepTester_filterbank.available()) {delay(100);};
      s->println("Press 'h' for help...");
      break;      
    case 'l':
      s->println("Command Received: toggle printing of per-band ave signal levels.");
      { bool as_dBSPL = false; togglePrintAveSignalLevels(as_dBSPL); }
      break;
    case 'L':
      s->println("Command Received: toggle printing of per-band ave signal levels.");
      { bool as_dBSPL = true; togglePrintAveSignalLevels(as_dBSPL); }
      break;
    case 'p':
      s->println("Command Received: enabling adaptive feedback cancelation.");
      feedbackCanceler.setEnable(true);
      //feedbackCanceler.resetAFCfilter();
      break;
    case 'P':
      s->println("Command Received: disabling adaptive feedback cancelation.");      
      feedbackCanceler.setEnable(false);
      break;
    case 'm':
      old_val = feedbackCanceler.getMu(); new_val = old_val * 2.0;
      s->print("Command received: increasing AFC mu to "); s->println(feedbackCanceler.setMu(new_val),6);
      break;
    case 'M':
      old_val = feedbackCanceler.getMu(); new_val = old_val / 2.0;
      s->print("Command received: decreasing AFC mu to "); s->println(feedbackCanceler.setMu(new_val),6);
      break;
    case 'r':
      old_val = feedbackCanceler.getRho(); new_val = 1.0-((1.0-old_val)/sqrt(2.0));
      s->print("Command received: increasing AFC rho to "); s->println(feedbackCanceler.setRho(new_val),6);
      break;
    case 'R':
      old_val = feedbackCanceler.getRho(); new_val = 1.0-((1.0-old_val)*sqrt(2.0));
      s->print("Command received: increasing AFC rho to "); s->println(feedbackCanceler.setRho(new_val),6);
      break;
    case 'e':
      old_val = feedbackCanceler.getEps(); new_val = old_val*sqrt(10.0);
      s->print("Command received: increasing AFC eps to "); s->println(feedbackCanceler.setEps(new_val),6);
      break;
    case 'E':
      old_val = feedbackCanceler.getEps(); new_val = old_val/sqrt(10.0);
      s->print("Command received: increasing AFC eps to "); s->println(feedbackCanceler.setEps(new_val),6);
      break;    
    case 'x':
      old_val = feedbackCanceler.getAfl(); new_val = old_val + 5;
      s->print("Command received: increasing AFC filter length to "); s->println(feedbackCanceler.setAfl(new_val));
      break;    
    case 'X':
      old_val = feedbackCanceler.getAfl(); new_val = old_val - 5;
      s->print("Command received: decreasing AFC filter length to "); s->println(feedbackCanceler.setAfl(new_val));
      break;            
//    case 'z':
//      old_val = feedbackCanceler.getNCoeffToZero(); new_val = old_val + 5;
//      s->print("Command received: increasing AFC N_Coeff_To_Zero to "); s->println(feedbackCanceler.setNCoeffToZero(new_val));
//      break;
//    case 'Z':
//      old_val = feedbackCanceler.getNCoeffToZero(); new_val = old_val - 5;
//      s->print("Command received: decreasing AFC N_Coeff_To_Zero to "); s->println(feedbackCanceler.setNCoeffToZero(new_val));      
//      break;
    case 'u':
    {
      old_val = audioHardware.getHPCutoff_Hz(); new_val = min(old_val*sqrt(2.0), 8000.0); //half-octave steps up
      float fs_Hz = audioHardware.getSampleRate_Hz();
      audioHardware.setHPFonADC(true,new_val,fs_Hz);
      s->print("Command received: Increasing ADC HP Cutoff to "); s->print(audioHardware.getHPCutoff_Hz());s->println(" Hz");
    }
      break;
    case 'U':
    {
      old_val = audioHardware.getHPCutoff_Hz(); new_val = max(old_val/sqrt(2.0), 5.0); //half-octave steps down
      float fs_Hz = audioHardware.getSampleRate_Hz();
      audioHardware.setHPFonADC(true,new_val,fs_Hz);
      s->print("Command received: Decreasing ADC HP Cutoff to "); s->print(audioHardware.getHPCutoff_Hz());s->println(" Hz");   
      break;
    }

  }
}

void SerialManager::incrementChannelGain(int chan, float change_dB) {
  if (chan < N_CHAN) {
    gain_algorithms[chan].incrementGain_dB(change_dB);
    //s->print("Incrementing gain on channel ");s->print(chan);
    //s->print(" by "); s->print(change_dB); s->println(" dB");
    printGainSettings();  //in main sketch file
  }
}

#endif

