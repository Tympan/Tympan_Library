

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
extern void switchToPCBMics(void);
extern void switchToMicInOnMicJack(void);
extern void switchToLineInOnMicJack(void);

extern bool set_NR_enable(bool val);
extern float increment_NR_attack_sec(float incr_fac);
extern float increment_NR_release_sec(float incr_fac); 
extern float increment_NR_max_atten_dB(float incr_dB);
extern float increment_NR_SNR_at_max_atten_dB(float incr_dB);
extern float increment_NR_transition_width_dB(float incr_dB);
extern float increment_NR_gain_smoothing_sec(float incr_fac);
extern bool set_NR_enable_noise_est_updates(bool val);

//now, define the Serial Manager class
class SerialManager : public SerialManagerBase {
  public:
    SerialManager(BLE *_ble) : SerialManagerBase(_ble) {  };
          
    void printHelp(void);

    void createTympanRemoteLayout(void); 
    void printTympanRemoteLayout(void); 

    void setFullGUIState(bool activeButtonsOnly = false);
    void setInputGainButtons(bool activeButtonsOnly = false);
    void setGainButtons(bool activeButtonsOnly = false);
    void setOutputGainButtons(bool activeButtonsOnly = false);
    void setNoiseRedButtons(bool activeButtonsOnly = false);
    void setNoiseLearningButtons(bool activeButtonsOnly = false);
        
    void setButtonText(String s1, String s2);
    void setButtonState(String btnId, bool newState, bool sendNow = true);
    
    int N_CHAN;
    float gainIncrement = 2.5f; 
    float attack_release_fac = 1.414213562f; //will be used to multiply or divide the current attack and release time
    float incr_atten_dB = 1.0;
    float incr_SNR_dB = 1.0;
    
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
  Serial.print(  "   k/K: Increase the digital gain (cur:"); Serial.print(myState.digital_gain_dB); Serial.println(" dB)");
  Serial.println(" Noise Reduction: No Prefix");
  Serial.print(  "   e/E: Enable/Disable NR (is_enabled = "); Serial.print(myState.NR_enable); Serial.println(")");
  Serial.print(  "   t/T: Raise/Lower learning attack (cur: "); Serial.print(myState.NR_attack_sec); Serial.println(" sec)");
  Serial.print(  "   r/R: Raise/Lower learning release (cur: "); Serial.print(myState.NR_release_sec); Serial.println(" sec)");
  Serial.print(  "   a/A: Raise/Lower max atten (cur: "); Serial.print(myState.NR_max_atten_dB); Serial.println(" dB)");
  Serial.print(  "   v/V: Raise/Lower SNR for max atten (cur: "); Serial.print(myState.NR_SNR_at_max_atten_dB); Serial.println( " dB)");
  Serial.print(  "   y/Y: Raise/Lower SNR transition width (cur: "); Serial.print(myState.NR_transition_width_dB); Serial.println( " dB)");
  Serial.print(  "   i/I: Raise/Lower gain smoothing (cur: "); Serial.print(myState.NR_gain_smooth_sec,3); Serial.println(" sec)");
  Serial.print(  "   u/U: Enable/Disable updating noise estimate (is_enabled = "); Serial.print(myState.NR_enable_noise_est_updates); Serial.println(")");
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
      incrementDigitalGain(gainIncrement); break;
      setGainButtons();
    case 'K':   //which is "shift k"
      incrementDigitalGain(-gainIncrement);  break;
      setGainButtons();
    case 'p':
      switchToPCBMics(); break;
    case 'm':
      switchToMicInOnMicJack(); break;
    case 'l':
      switchToLineInOnMicJack();break;
    case 'e':
      { bool new_val = set_NR_enable(true);
      Serial.print("Recieved: enable noise reduction = "); Serial.println(new_val);}
      setNoiseRedButtons();
      break;
    case 'E':
      { bool new_val = set_NR_enable(false);
      Serial.print("Recieved: enable noise reduction = "); Serial.println(new_val);}
      setNoiseRedButtons();;
      break;      
    case 't':
      { float new_val = increment_NR_attack_sec(attack_release_fac);
      Serial.print("Recieved: new attack (sec) = "); Serial.println(new_val);}
      setNoiseLearningButtons();
      break;
    case 'T':
      { float new_val = increment_NR_attack_sec(1.0/attack_release_fac);
      Serial.print("Recieved: new attack (sec) = "); Serial.println(new_val);}
      setNoiseLearningButtons();
      break;
    case 'r':
      { float new_val = increment_NR_release_sec(attack_release_fac);
      Serial.print("Recieved: new release (sec) = "); Serial.println(new_val);}
      setNoiseLearningButtons();
      break;
    case 'R':
      { float new_val = increment_NR_release_sec(1.0/attack_release_fac);
      Serial.print("Recieved: new release (sec) = "); Serial.println(new_val);}
      setNoiseLearningButtons();
      break;    
    case 'a':
      { float new_val = increment_NR_max_atten_dB(incr_atten_dB);
      Serial.print("Recieved: new max attenuation (dB) = "); Serial.println(new_val);}
      setNoiseRedButtons();
      break;
    case 'A':
      { float new_val = increment_NR_max_atten_dB(-incr_atten_dB);
      Serial.print("Recieved: new max attenuation (dB) = "); Serial.println(new_val);}
      setNoiseRedButtons();;
      break;
     case 'v':
      { float new_val = increment_NR_SNR_at_max_atten_dB(incr_SNR_dB);
      Serial.print("Recieved: new SNR at max atten (dB) = "); Serial.println(new_val);}
      setNoiseRedButtons();
      break;
    case 'V':
      { float new_val = increment_NR_SNR_at_max_atten_dB(-incr_SNR_dB);
      Serial.print("Recieved: new SNR  at max atten (dB) = "); Serial.println(new_val);}
      setNoiseRedButtons();;
      break;
     case 'y':
      { float new_val = increment_NR_transition_width_dB(incr_SNR_dB);
      Serial.print("Recieved: new transition width (dB) = "); Serial.println(new_val);}
      setNoiseRedButtons();
      break;
    case 'Y':
      { float new_val = increment_NR_transition_width_dB(-incr_SNR_dB);
      Serial.print("Recieved: new transition width (dB) = "); Serial.println(new_val);}
      setNoiseRedButtons();;
      break;
    case 'i':
      { float new_val = increment_NR_gain_smoothing_sec(attack_release_fac);
      Serial.print("Recieved: new gain smoothing (sec) = "); Serial.println(new_val,3);}
      setNoiseRedButtons();
      break;
    case 'I':
      { float new_val = increment_NR_gain_smoothing_sec(1.0/attack_release_fac);
      Serial.print("Recieved: new gain smoothing (sec) = "); Serial.println(new_val,3);}
      setNoiseRedButtons();
      break;            
    case 'u':
      { bool new_val = set_NR_enable_noise_est_updates(true);
      Serial.print("Recieved: enable noise esimation updates = "); Serial.println(new_val);}
      setNoiseLearningButtons();
      break;
    case 'U':
      { bool new_val = set_NR_enable_noise_est_updates(false);
      Serial.print("Recieved: enable noise esimation updates = "); Serial.println(new_val);}
      setNoiseLearningButtons();;
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
  page_h = myGUI.addPage("Noise Reduction");
  
    card_h = page_h->addCard("Digital Gain (dB)");
      card_h->addButton("-", "K", "",          4);  //displayed string, command, button ID, button width (out of 12)
      card_h->addButton("",  "",  "digGain",   4);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
      card_h->addButton("+", "k", "",          4);   //displayed string, command, button ID, button width (out of 12)

    card_h = page_h->addCard("NR: Enable");  
      card_h->addButton("Enable", "e", "NRenable",6);  //label, command, id, width
      card_h->addButton("Disable", "E", "",       6);  //label, command, id, width //display the formant shift value
      
    card_h = page_h->addCard("NR: Max Atten (dB)");  
      card_h->addButton("-", "A", "",          4);  //label, command, id, width
      card_h->addButton("",  "",  "NRatten",   4);  //label, command, id, width //display the formant shift value
      card_h->addButton("+", "a", "",          4);  //label, command, id, width

    card_h = page_h->addCard("NR: SNR for Max Atten (dB)");  
      card_h->addButton("-", "v", "",          4);  //label, command, id, width
      card_h->addButton("",  "",  "NRthresh",  4);  //label, command, id, width //display the formant shift value
      card_h->addButton("+", "V", "",          4);  //label, command, id, width

    card_h = page_h->addCard("NR: Transition Width (dB)");  
      card_h->addButton("-", "y", "",          4);  //label, command, id, width
      card_h->addButton("",  "",  "NRtrans",  4);  //label, command, id, width //display the formant shift value
      card_h->addButton("+", "Y", "",          4);  //label, command, id, width

    card_h = page_h->addCard("NR: Gain Smoothing (sec)");  
      card_h->addButton("-", "i", "",          4);  //label, command, id, width
      card_h->addButton("",  "",  "NRsmooth",  4);  //label, command, id, width //display the formant shift value
      card_h->addButton("+", "I", "",          4);  //label, command, id, width

        

  page_h = myGUI.addPage("Noise Learning");

    card_h = page_h->addCard("Learn Noise");  
      card_h->addButton("Enable", "u", "NRupdate",6);  //label, command, id, width
      card_h->addButton("Disable", "U", "",       6);  //label, command, id, width //display the formant shift value

    card_h = page_h->addCard("Attack (sec)");  
      card_h->addButton("-", "T", "",          4);  //label, command, id, width
      card_h->addButton("",  "",  "NRatt",     4);  //label, command, id, width //display the formant shift value
      card_h->addButton("+", "t", "",          4);  //label, command, id, width
      
    card_h = page_h->addCard("Release (sec)");  
      card_h->addButton("-", "R", "",          4);  //label, command, id, width
      card_h->addButton("",  "",  "NRrel",     4);  //label, command, id, width //display the formant shift value
      card_h->addButton("+", "r", "",          4);  //label, command, id, width

      
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
  setNoiseRedButtons(activeButtonsOnly);
  setNoiseLearningButtons(activeButtonsOnly);
  setInputGainButtons(activeButtonsOnly);
  setGainButtons(activeButtonsOnly);
  setOutputGainButtons(activeButtonsOnly);
  if (use_ble) SerialManagerBase::setFullGUIState(activeButtonsOnly);
}

void SerialManager::setInputGainButtons(bool activeButtonsOnly) { setButtonText("inGain", String(myState.input_gain_dB)); };
void SerialManager::setGainButtons(bool activeButtonsOnly) {  setButtonText("digGain", String(myState.digital_gain_dB)); }
void SerialManager::setOutputGainButtons(bool activeButtonsOnly) {  setButtonText("outGain", String(myState.output_gain_dB)); }

void SerialManager::setNoiseRedButtons(bool activeButtonsOnly) {
  if ((!activeButtonsOnly) || (myState.NR_enable)) setButtonState("NRenable",myState.NR_enable);
  setButtonText("NRatten",String(myState.NR_max_atten_dB,1));
  setButtonText("NRthresh",String(myState.NR_SNR_at_max_atten_dB,1));
  setButtonText("NRtrans",String(myState.NR_transition_width_dB,1));
  setButtonText("NRsmooth",String(myState.NR_gain_smooth_sec,3));
}
void SerialManager::setNoiseLearningButtons(bool activeButtonsOnly) {
  if ((!activeButtonsOnly) || (myState.NR_enable_noise_est_updates)) setButtonState("NRupdate",myState.NR_enable_noise_est_updates);
  setButtonText("NRatt", String(myState.NR_attack_sec,2));
  setButtonText("NRrel", String(myState.NR_release_sec,2));
} 

void SerialManager::setButtonText(String s1, String s2) {  if (use_ble) SerialManagerBase::setButtonText(s1,s2); }
void SerialManager::setButtonState(String btnId, bool newState, bool sendNow){  if (use_ble) SerialManagerBase::setButtonState(btnId, newState, sendNow);}


#endif
