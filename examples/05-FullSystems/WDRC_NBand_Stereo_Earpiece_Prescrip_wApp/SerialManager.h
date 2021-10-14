
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>
#include "State.h"

//classes from the main sketch that might be used here
extern Tympan myTympan;               //defined in main *.ino file
extern State myState;
extern EarpieceMixer_F32_UI earpieceMixer; //created in the main *.ino file
extern AudioSDWriter_F32_UI audioSDWriter;

#if USE_FIR_FILTERBANK
extern AudioEffectMultiBandWDRC_F32_UI multiBandWDRC[2];
#else
extern AudioEffectMultiBandWDRC_IIR_F32_UI multiBandWDRC[2];
#endif
extern StereoContainerWDRC_UI stereoContainerWDRC; 
extern BTNRH_StereoPresetManager_UI presetManager;

extern const int N_CHAN;
extern AudioControlTestAmpSweep_F32 ampSweepTester;
extern AudioControlTestFreqSweep_F32 freqSweepTester;
//extern AudioControlTestFreqSweep_F32 freqSweepTester_filterbank;

//functions in the main sketch that I want to call from here
extern float incrementDigitalGain(float);
extern void printGainSettings(void);
extern void togglePrintAveSignalLevels(bool);
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

    float channelGainIncrement_dB = 2.5f;  
  private:

    TympanRemoteFormatter myGUI;  //Creates the GUI-writing class for interacting with TympanRemote App
   
};

void SerialManager::printHelp(void) {  
  Serial.println();
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println(F(" General: No Prefix"));
  Serial.println(F("   h: Print this help"));
  Serial.println(F("   g: Print the gain settings of the device."));
  //Serial.println("   c/C: Enable/Disable printing of CPU and Memory usage");
  Serial.println(F("   l: Toggle printing of pre-gain per-channel signal levels (dBFS)"));
  Serial.println(F("   L: Toggle printing of pre-gain per-channel signal levels (dBSPL, per DSL 'maxdB')"));
  Serial.println(F("   A: Self-Generated Test: Amplitude sweep.  End-to-End Measurement."));
  Serial.println(F("   F: Self-Generated Test: Frequency sweep.  End-to-End Measurement."));
  //Serial.println(F("   f: Self-Generated Test: Frequency sweep.  Measure filterbank."));
  Serial.print(F(  "   k/K: Incr/Decrease overall gain by ")); Serial.print(channelGainIncrement_dB); Serial.println(" dB");
  //Serial.println(F("   d/D: Switch between presets: NORMAL vs FULL-ON"));
  //Serial.println(F(" r/s: Begin/stop SD audio recording"));
  SerialManagerBase::printHelp();  ////in here, it automatically loops over the different UI elements issuing printHelp()
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
//    case 'd':
//      Serial.println("Command Received: changing to 1st prescription...");
//      switchPrescription(1-1);
//      setFullGUIState(); //resend all the algorithm parameter values to the App's GUI
//      break;
//    case 'D':
//      Serial.println("Command Received: changing to 2nd prescription...");
//      switchPrescription(2-1);
//      setFullGUIState(); //resend all the algorithm parameter values to the App's GUI
//      break;
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
//    case 'f':
//      //frequency sweep test
//      { //limit the scope of any variables that I create here
//        freqSweepTester_filterbank.setSignalAmplitude_dBFS(-30.f);
//        float start_freq_Hz = 125.0f, end_freq_Hz = 12000.f, step_octave = powf(2.0,1.0/6.0); //pow(2.0,1.0/3.0) is 3 steps per octave
//        freqSweepTester_filterbank.setStepPattern(start_freq_Hz, end_freq_Hz, step_octave);
//        freqSweepTester_filterbank.setTargetDurPerStep_sec(0.5);
//      }
//      Serial.println("Command Received: starting test using frequency sweep.  Filterbank assessment...");
//      freqSweepTester_filterbank.begin();
//      while (!freqSweepTester_filterbank.available()) {delay(100);};
//      Serial.println("Press 'h' for help...");
//      break;      
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
    default:
      ret_val = SerialManagerBase::processCharacter(c);  //in here, it automatically loops over the different UI elements
      break; 
  }
  return ret_val;
}


// Print the layout for the Tympan Remote app, in a JSON-ish string
void SerialManager::printTympanRemoteLayout(void) {
    bool is_GUI_created_here = false;
    if (myGUI.get_nPages() < 1) { createTympanRemoteLayout(); is_GUI_created_here=true; }  //create the GUI, if it hasn't already been created
    String s = myGUI.asString();                      //compose the string that holds the JSON describing the GUI
    if (is_GUI_created_here) myGUI.deleteAllPages();  //myGUI might use a fair bit of RAM...so free up the RAM, if we can
    Serial.println(s);                                //print the JSON for the GUI to the USB Serial (for debugging)
    ble->sendMessage(s);                              //print the JSON to Bluetooth (ble is held by SerialManagerBase)
    setFullGUIState();                                //send all the state values to fill in the GUI's blank spaces
}

//define the GUI for the App
void SerialManager::createTympanRemoteLayout(void) {
  
  // Create some temporary variables
  TR_Page *page_h;  //dummy handle for a page
  TR_Card *card_h;  //dummy handle for a card

  //Add first page to GUI
  page_h = myGUI.addPage("WDRC " + String(N_CHAN) + "-Band System");

      //Add a button group ("card") for Volume
      card_h = page_h->addCard("Volume Knob (dB)");
          //Add a "-" digital gain button with the Label("-"); Command("K"); Internal ID (none needed); and width (4)
          card_h->addButton("-", "K", "",   4);  //displayed string, command, button ID, button width (out of 12)

          //Add an indicator that's a button with no command:  Label (value of the digital gain); Command (""); Internal ID ("gain"); width (4).
          card_h->addButton("",  "", "gain",4);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
  
          //Add a "+" digital gain button with the Label("+"); Command("K"); Internal ID (none needed); and width (4)
          card_h->addButton("+", "k", "",   4);  //displayed string, command, button ID, button width (out of 12)

//      //Add a button group ("card") for Presets
//      card_h = page_h->addCard("Parameter Presets");
//          //Add two buttons for the two presets
//          card_h->addButton("Normal",      "d", "preset0", 6);  //displayed string, command, button ID, button width (out of 12)
//          card_h->addButton("Full-On Gain","D", "preset1", 6);  //displayed string, command, button ID, button width (out of 12)
      
      //Add a button group ("card") for the CPU reporting...use a button set that is built into myState for you!
      card_h = myState.addCard_cpuReporting(page_h);
      
      //Add a button group for SD recording...use a button set that is built into AudioSDWriter_F32_UI for you!
      card_h = audioSDWriter.addCard_sdRecord(page_h);

  // add page for earpiece mixing
  page_h = myGUI.addPage("Earpiece Mixer");
      //select inputs
      card_h = earpieceMixer.addCard_audioSource(page_h); //use its predefined group of buttons for input audio source
      
  //Add second page for more control of earpieces
  page_h = earpieceMixer.addPage_digitalEarpieces(&myGUI); //use its predefined page for controlling the digital earpieces

  //add MultiBand WDRC pages
  page_h = stereoContainerWDRC.addPage_filterbank(&myGUI);
  page_h = stereoContainerWDRC.addPage_compressorbank_globals(&myGUI);
  page_h = stereoContainerWDRC.addPage_compressorbank_perBand(&myGUI);
  page_h = stereoContainerWDRC.addPage_compressor_broadband(&myGUI);

  //add page for saving and recalling presets
  page_h = presetManager.addPage_default(&myGUI);
        
  //add some pre-defined pages to the GUI...these are pre-defined within the App itself
  myGUI.addPredefinedPage("serialMonitor");
  //myGUI.addPredefinedPage("serialPlotter");
}

void SerialManager::updateGainDisplay(void) {
  setButtonText("gain",String(myState.digital_gain_dB,1)); //send a new string to the button with id "gain"
}

//void SerialManager::updatePresetDisplay(void) {
//  for (int i=0; i< myState.n_prescriptions; i++) {
//    String id = String("preset") + String(i);
//    bool state = false;
//    if (i==myState.current_prescription_ind) state=true;
//    setButtonState(id,state);
//  }
//}

void SerialManager::setFullGUIState(bool activeButtonsOnly) {
  //updatePresetDisplay();
  updateGainDisplay();

  //update all of the individual UI elements
  SerialManagerBase::setFullGUIState(activeButtonsOnly); //in here, it automatically loops over the different UI elements
}

#endif
