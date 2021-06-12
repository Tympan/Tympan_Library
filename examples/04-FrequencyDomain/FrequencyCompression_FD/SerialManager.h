

#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>
#include "State.h"

//extern objects
extern State myState;
extern const bool use_ble;

//functions in the main sketch that I want to call from here
extern void incrementDigitalGain(float);
extern void printGainSettings(void);
extern float incrementFreqKnee(float);
extern float incrementFreqCR(float);
extern float incrementFreqShift(float);
extern void switchToPCBMics(void);
extern void switchToMicInOnMicJack(void);
extern void switchToLineInOnMicJack(void);

//now, define the Serial Manager class
class SerialManager : public SerialManagerBase {
  public:
    SerialManager(BLE *_ble) : SerialManagerBase(_ble) {  };
          
    void printHelp(void);

    void createTympanRemoteLayout(void); //TBD
    void printTympanRemoteLayout(void);  //TBD

    void setFullGUIState(bool activeButtonsOnly = false);
    void setInputGainButtons(bool activeButtonsOnly = false);
    void setGainButtons(bool activeButtonsOnly = false);
    void setOutputGainButtons(bool activeButtonsOnly = false);
    void setNLFreqParams(bool activeButtonsOnly = false);
    void setButtonText(String s1, String s2);
    
    int N_CHAN;
    float channelGainIncrement_dB = 2.5f; 
    float freq_knee_increment_Hz = 100.0;
    float freq_shift_increment_Hz = 100.0;
    float freq_cr_increment = powf(2.0,1.0/6.0);
    
  private:
    TympanRemoteFormatter myGUI;  //Creates the GUI-writing class for interacting with TympanRemote App
    virtual bool processCharacter(char c);
    bool firstTimeSendingGUI = true;
};

void SerialManager::printHelp(void) {  
  Serial.println();
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println(" General: No Prefix");
  Serial.println("   h: Print this help");
  Serial.println("   g: Print the gain settings of the device.");
  Serial.println("   p: Switch to built-in PCB microphones");
  Serial.println("   m: switch to external mic via mic jack");
  Serial.println("   l: switch to line-in via mic jack");
  Serial.print(  "   k/K: Increase the gain of all channels (ie, knob gain) by "); Serial.print(channelGainIncrement_dB); Serial.println(" dB");
  Serial.print(  "   t/T: Raise/Lower freq knee (change by "); Serial.print(freq_knee_increment_Hz); Serial.println(" Hz)");
  Serial.print(  "   r/R: Raise/Lower freq compression (change by "); Serial.print(freq_cr_increment); Serial.println("x)");
  Serial.print(  "   f/F: Raise/Lower freq shifting (change by "); Serial.print(freq_shift_increment_Hz); Serial.println(" Hz)");
  SerialManagerBase::printHelp();  ////in here, it automatically loops over the different UI elements issuing printHelp()
}


//switch yard to determine the desired action
bool SerialManager::processCharacter(char c) { //this is called by SerialManagerBase::respondToByte()
  //float old_val = 0.0, new_val = 0.0;
  bool return_val = true; //assume we'll use the given character
  switch (c) {
    case 'h': case '?':
      printHelp(); break;
    case 'g': case 'G':
      printGainSettings(); break;
    case 'k':
      incrementDigitalGain(channelGainIncrement_dB); break;
      setGainButtons();
    case 'K':   //which is "shift k"
      incrementDigitalGain(-channelGainIncrement_dB);  break;
      setGainButtons();
    case 'p':
      switchToPCBMics(); break;
    case 'm':
      switchToMicInOnMicJack(); break;
    case 'l':
      switchToLineInOnMicJack();break;
    case 't':
      { float new_val = incrementFreqKnee(freq_knee_increment_Hz);
      Serial.print("Recieved: new freq knee (Hz) = "); Serial.println(new_val);}
      setNLFreqParams();
      break;
    case 'T':
      { float new_val = incrementFreqKnee(-freq_knee_increment_Hz);
      Serial.print("Recieved: new freq knee (Hz) = "); Serial.println(new_val);}
      setNLFreqParams();
      break;
    case 'r':
      { float new_val = incrementFreqCR(freq_cr_increment);
      Serial.print("Recieved: new freq CompRatio = "); Serial.println(new_val);}
      setNLFreqParams();
      break;
    case 'R':
      { float new_val = incrementFreqCR(1.0/freq_cr_increment);
      Serial.print("Recieved: new freq CompRatio = "); Serial.println(new_val);}
      setNLFreqParams();
      break;    
    case 'f':
      { float new_val = incrementFreqShift(freq_shift_increment_Hz);
      Serial.print("Recieved: new freq shift (Hz) = "); Serial.println(new_val);}
      setNLFreqParams();
      break;
    case 'F':
      { float new_val = incrementFreqShift(-freq_shift_increment_Hz);
      Serial.print("Recieved: new freq shift (Hz) = "); Serial.println(new_val);}
      setNLFreqParams();
      break;
    case 'J':
      printTympanRemoteLayout();
      setFullGUIState(firstTimeSendingGUI);
      firstTimeSendingGUI = false;
      break;      
    default:
      return_val = SerialManagerBase::processCharacter(c);  //in here, it automatically loops over the different UI elements
      break;       
  }
  return return_val;
}


// Print the layout for the Tympan Remote app, in a JSON-ish string
void SerialManager::createTympanRemoteLayout(void) {
  // Create some temporary variables
  TR_Page *page_h;  //dummy handle for a page
  TR_Card *card_h;  //dummy handle for a card

  //Add first page to GUI
  page_h = myGUI.addPage("Non-Linear Freq Lowering");
  
    card_h = page_h->addCard("Digital Gain (dB)");
      card_h->addButton("-", "K", "",          4);  //displayed string, command, button ID, button width (out of 12)
      card_h->addButton("",  "",  "digGain",   4);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
      card_h->addButton("+", "k", "",          4);   //displayed string, command, button ID, button width (out of 12)
      
    card_h = page_h->addCard("Start of Processing (Knee, Hz)");  
      card_h->addButton("-", "T", "",          4);  //label, command, id, width
      card_h->addButton("",  "",  "freqKnee",  4);  //label, command, id, width //display the formant shift value
      card_h->addButton("+", "t", "",          4);  //label, command, id, width
      
    card_h = page_h->addCard("Compression Ratio (x)");  
      card_h->addButton("-", "R", "",          4);  //label, command, id, width
      card_h->addButton("",  "",  "freqCR",    4);  //label, command, id, width //display the formant shift value
      card_h->addButton("+", "r", "",          4);  //label, command, id, width

    card_h = page_h->addCard("Frequency Shifting (Hz)");  
      card_h->addButton("-", "F", "",          4);  //label, command, id, width
      card_h->addButton("",  "",  "freqShift", 4);  //label, command, id, width //display the formant shift value
      card_h->addButton("+", "f", "",          4);  //label, command, id, width

  //add second page to GUI
  page_h = myGUI.addPage("Globals");
  
    card_h = myState.addCard_cpuReporting(page_h);

    card_h = page_h->addCard("Input Gain (dB)");
      card_h->addButton("-", "", "",        4);   //displayed string, command, button ID, button width (out of 12)
      card_h->addButton("",  "", "inGain",  4);   //displayed string (blank for now), command (blank), button ID, button width (out of 12)
      card_h->addButton("+", "", "",        4);   //displayed string, command, button ID, button width (out of 12)

    card_h = page_h->addCard("Volume Wheel(dB)");
      card_h->addButton("-", "", "",        4);  //displayed string, command, button ID, button width (out of 12)
      card_h->addButton("",  "", "outGain", 4);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
      card_h->addButton("+", "", "",        4);  //displayed string, command, button ID, button width (out of 12)

  //myGUI.addPredefinedPage("serialMonitor");
}


void SerialManager::printTympanRemoteLayout(void) {
    String s = myGUI.asString();
    Serial.println(s);
    ble->sendMessage(s); //ble is held by SerialManagerBase
}

void SerialManager::setFullGUIState(bool activeButtonsOnly) {
  setInputGainButtons(activeButtonsOnly);
  setGainButtons(activeButtonsOnly);
  setOutputGainButtons(activeButtonsOnly);
  
  setNLFreqParams(activeButtonsOnly);
}
void SerialManager::setInputGainButtons(bool activeButtonsOnly) {
   setButtonText("inGain", String(myState.input_gain_dB)); 
};
void SerialManager::setGainButtons(bool activeButtonsOnly) {
  setButtonText("digGain", String(myState.digital_gain_dB));
}
void SerialManager::setOutputGainButtons(bool activeButtonsOnly) {
  setButtonText("outGain", String(myState.output_gain_dB));
}
void SerialManager::setNLFreqParams(bool activeButtonsOnly) {
  setButtonText("freqKnee", String(myState.freq_knee_Hz,0));
  setButtonText("freqCR",   String(myState.freq_knee_Hz,2));
  setButtonText("freqShift",String(myState.freq_knee_Hz,1));
}

void SerialManager::setButtonText(String s1, String s2) {
  if (use_ble) SerialManagerBase::setButtonText(s1,s2);
}


#endif
