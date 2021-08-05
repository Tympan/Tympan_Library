
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>
#include "State.h"

//classes from the main sketch that might be used here
extern Tympan myTympan;               //defined in main *.ino file
extern State myState;
extern const int N_CHAN;
extern AudioEffectCompWDRC_F32 expCompLim[];
extern AudioControlTestAmpSweep_F32 ampSweepTester;
extern AudioControlTestFreqSweep_F32 freqSweepTester;
extern AudioControlTestFreqSweep_F32 freqSweepTester_filterbank;

//functions in the main sketch that I want to call from here
extern float incrementDigitalGain(float);
extern void printGainSettings(void);
extern void togglePrintAveSignalLevels(bool);
extern int incrementDSLConfiguration(void);
extern float incrementChannelGain(int, float);


//now, define the Serial Manager class
class SerialManager : public SerialManagerBase  {  // see Tympan_Library SerialManagerBase for more functions!
  public:
    SerialManager(BLE *_ble) : SerialManagerBase(_ble) {};
      
    void printHelp(void);
    void createTympanRemoteLayout(void); 
    void printTympanRemoteLayout(void); 
    void setFullGUIState(bool activeButtonsOnly = false);
    bool processCharacter(char c);  //this is called by SerialManagerBase.respondToByte(char c)
    void updateGainDisplay(void);
    void setSDRecordingButtons(void);

    float channelGainIncrement_dB = 2.5f;  
  private:

    TympanRemoteFormatter myGUI;  //Creates the GUI-writing class for interacting with TympanRemote App
   
};

#define MAX_CHANS 8
void printChanUpMsg(int N_CHAN) {
  char fooChar[] = "12345678";
  Serial.print(" ");
  for (int i=0;i<min(MAX_CHANS,N_CHAN);i++) {
    Serial.print(fooChar[i]); 
    if (i < (N_CHAN-1)) Serial.print(",");
  }
  Serial.print(": Increase linear gain of given channel (1-");
  Serial.print(N_CHAN);
  Serial.print(") by ");
}
void printChanDownMsg(int N_CHAN) {
  char fooChar[] = "!@#$%^&*";
  Serial.print(" ");
  for (int i=0;i<min(MAX_CHANS,N_CHAN);i++) {
    Serial.print(fooChar[i]); 
    if (i < (N_CHAN-1)) Serial.print(",");
  }
  Serial.print(": Decrease linear gain of given channel (1-");
  Serial.print(N_CHAN);
  Serial.print(") by ");
}
void SerialManager::printHelp(void) {  
  Serial.println();
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println(" h: Print this help");
  Serial.println(" g: Print the gain settings of the device.");
  Serial.println(" c/C: Enable/Disable printing of CPU and Memory usage");
  Serial.println(" l: Toggle printing of pre-gain per-channel signal levels (dBFS)");
  Serial.println(" L: Toggle printing of pre-gain per-channel signal levels (dBSPL, per DSL 'maxdB')");
  Serial.println(" A: Self-Generated Test: Amplitude sweep.  End-to-End Measurement.");
  Serial.println(" F: Self-Generated Test: Frequency sweep.  End-to-End Measurement.");
  Serial.println(" f: Self-Generated Test: Frequency sweep.  Measure filterbank.");
  Serial.print(" k: Increase the gain of all channels (ie, knob gain) by "); Serial.print(channelGainIncrement_dB); Serial.println(" dB");
  Serial.print(" K: Decrease the gain of all channels (ie, knob gain) by "); Serial.print(channelGainIncrement_dB); Serial.println(" dB");
  printChanUpMsg(N_CHAN);  Serial.print(channelGainIncrement_dB); Serial.println(" dB");
  printChanDownMsg(N_CHAN);  Serial.print(channelGainIncrement_dB); Serial.println(" dB");
  Serial.println(" D: Toggle between DSL configurations: NORMAL vs FULL-ON");
  Serial.println(" r/s: Begin/stop SD audio recording");
  Serial.println();
}


//switch yard to determine the desired action
bool SerialManager::processCharacter(char c) {  //this is called by SerialManagerBase.respondToByte(char c)
  bool ret_val = true;
  float old_val = 0.0, new_val = 0.0;
  switch (c) {
    case 'h': case '?':
      printHelp(); break;
    case 'J': case 'j':
      printTympanRemoteLayout();
      break;      
    case 'g': case 'G':
      printGainSettings(); break;
    case 'k':
      incrementDigitalGain(channelGainIncrement_dB); break;
    case 'K':   //which is "shift k"
      incrementDigitalGain(-channelGainIncrement_dB);  break;
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
      Serial.println("Command Received: starting test using amplitude sweep...");
      ampSweepTester.begin();
      while (!ampSweepTester.available()) {delay(100);};
      Serial.println("Press 'h' for help...");
      break;
    case 'c':      
      Serial.println("Received: printing memory and CPU usage.");
      myState.flag_printCPUandMemory = true;
      setButtonState("cpuStart",myState.flag_printCPUandMemory);
      break;     
    case 'C':
      Serial.println("Received: stopping printing memory and CPU usage.");
      myState.flag_printCPUandMemory = false;
      setButtonState("cpuStart",myState.flag_printCPUandMemory);
      break;     
    case 'D':
      Serial.println("Command Received: changing DSL configuration...you will lose any custom gain values...");
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
      Serial.println("Command Received: starting test using frequency sweep, end-to-end assessment...");
      freqSweepTester.begin();
      while (!freqSweepTester.available()) {delay(100);};
      Serial.println("Press 'h' for help...");
      break; 
    case 'f':
      //frequency sweep test
      { //limit the scope of any variables that I create here
        freqSweepTester_filterbank.setSignalAmplitude_dBFS(-30.f);
        float start_freq_Hz = 125.0f, end_freq_Hz = 12000.f, step_octave = powf(2.0,1.0/6.0); //pow(2.0,1.0/3.0) is 3 steps per octave
        freqSweepTester_filterbank.setStepPattern(start_freq_Hz, end_freq_Hz, step_octave);
        freqSweepTester_filterbank.setTargetDurPerStep_sec(0.5);
      }
      Serial.println("Command Received: starting test using frequency sweep.  Filterbank assessment...");
      freqSweepTester_filterbank.begin();
      while (!freqSweepTester_filterbank.available()) {delay(100);};
      Serial.println("Press 'h' for help...");
      break;      
    case 'l':
      Serial.println("Command Received: toggle printing of per-band ave signal levels.");
      { bool as_dBSPL = false; togglePrintAveSignalLevels(as_dBSPL); }
      break;
    case 'L':
      Serial.println("Command Received: toggle printing of per-band ave signal levels.");
      { bool as_dBSPL = true; togglePrintAveSignalLevels(as_dBSPL); }
      break; 
    case 'u':
    {
      old_val = myTympan.getHPCutoff_Hz(); new_val = min(old_val*sqrt(2.0), 8000.0); //half-octave steps up
      float fs_Hz = myTympan.getSampleRate_Hz();
      myTympan.setHPFonADC(true,new_val,fs_Hz);
      Serial.print("Command received: Increasing ADC HP Cutoff to "); Serial.print(myTympan.getHPCutoff_Hz());Serial.println(" Hz");
    }
      break;
    case 'U':
    {
      old_val = myTympan.getHPCutoff_Hz(); new_val = max(old_val/sqrt(2.0), 5.0); //half-octave steps down
      float fs_Hz = myTympan.getSampleRate_Hz();
      myTympan.setHPFonADC(true,new_val,fs_Hz);
      Serial.print("Command received: Decreasing ADC HP Cutoff to "); Serial.print(myTympan.getHPCutoff_Hz());Serial.println(" Hz");   
      break;
    }
    case 'r':
      Serial.println("Received: begin SD recording");
      audioSDWriter.startRecording();
      setSDRecordingButtons();
      break;
    case 's':
      Serial.println("Received: stop SD recording");
      audioSDWriter.stopRecording();
      setSDRecordingButtons();
      break;    
    default:
      ret_val = false;
  }
  return ret_val;
}


// Print the layout for the Tympan Remote app, in a JSON-ish string
void SerialManager::printTympanRemoteLayout(void) {
    if (myGUI.get_nPages() < 1) createTympanRemoteLayout();  //create the GUI, if it hasn't already been created
    String s = myGUI.asString();
    Serial.println(s);
    ble->sendMessage(s); //ble is held by SerialManagerBase
    setFullGUIState();
}

//define the GUI for the App
void SerialManager::createTympanRemoteLayout(void) {
  
  // Create some temporary variables
  TR_Page *page_h;  //dummy handle for a page
  TR_Card *card_h;  //dummy handle for a card

  //Add first page to GUI
  page_h = myGUI.addPage("WDRC 8-Band, Full System");
  
      //Add a button group ("card")
      card_h = page_h->addCard("Volume Knob (dB)");
          //Add a "-" digital gain button with the Label("-"); Command("K"); Internal ID ("minusButton"); and width (4)
          card_h->addButton("-","K","minusButton",4);  //displayed string, command, button ID, button width (out of 12)

          //Add an indicator that's a button with no command:  Label (value of the digital gain); Command (""); Internal ID ("gain indicator"); width (4).
          card_h->addButton("","","gainIndicator",4);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
  
          //Add a "+" digital gain button with the Label("+"); Command("K"); Internal ID ("minusButton"); and width (4)
          card_h->addButton("+","k","plusButton",4);   //displayed string, command, button ID, button width (out of 12)

      //Add a button group ("card") for the CPU reporting...use a button set that is built into myState for you!
      myState.addCard_cpuReporting(page_h, String("")); //see TympanStateBase.h  assumes use of 'c' and 'C' for on and off

      //Add a button group for SD recording...use a button set that is built into AudioSDWriter_F32_UI for you!
      audioSDWriter.addCard_sdRecord(page_h, String(""));  //see AudioSDwriter_F32.h  assumes use of 'r' and 's'
        
  //add some pre-defined pages to the GUI...these are pre-defined within the App itself
  myGUI.addPredefinedPage("serialMonitor");
  //myGUI.addPredefinedPage("serialPlotter");
}

void SerialManager::setFullGUIState(bool activeButtonsOnly) {
  updateGainDisplay();
  myState.setCPUButtons(); //see TympanStateBase.h...it's a built-in update routine for the built-in CPU buttons that I created earlier
  setSDRecordingButtons();  
}

void SerialManager::updateGainDisplay(void) {
  setButtonText("gainIndicator",String(myState.digital_gain_dB,1));
}

void SerialManager::setSDRecordingButtons(void) {
  if (audioSDWriter.getState() == AudioSDWriter_F32::STATE::RECORDING) {
    setButtonState("recordStart",true);
  } else {
    setButtonState("recordStart",false);
  }
  setButtonText("sdFname",audioSDWriter.getCurrentFilename());
}

#endif
