
#include "EarpieceMixer_F32_UI.h"

void EarpieceMixer_F32_UI::printHelp(void) {
  Serial.println(F(" EarpieceMixer: Prefix = ") + String(quadchar_start_char) + String(ID_char) + String("x"));
  Serial.println(F("   w/W/e/E/d: Inputs: PCB Mics, Mic on Jack, Line on Jack, BT, PDM Earpieces"));
  Serial.println(F("   t/T/H: Inputs: Front Mic, Inverted-Delayed Mix of Mics, or Rear Mic"));
  Serial.print  (F("   u/U: Front Mic: incr/decr delay by one sample (currently ")); Serial.print(state.currentFrontDelay_samps); Serial.println(")");
  Serial.print  (F("   y/Y: Rear Mic: incr/decr delay by one sample (currently ")); Serial.print(state.currentRearDelay_samps); Serial.println(")");
  Serial.print  (F("   i/I: Rear Mic: incr/decr gain by ")); Serial.print(rearMicGainIncrement_dB); Serial.println(" dB");
  Serial.println(F("   q,Q: Mute or Unmute the audio."));
  Serial.println(F("   s,S,B: Left, Right, or Mono-mix the audio"));
}


bool EarpieceMixer_F32_UI::processCharacterTriple(char mode_char, char chan_char, char data_char) {
  bool return_val = false;
  if (mode_char != ID_char) return return_val;

  //we ignore the chan_char and only work with the data_char
  char c = data_char;
  
  //switch yard
  return_val = true;
  switch (c) {    
    case 'w':
      Serial.println("EarpieceMixer_F32_UI: Received: Listen to PCB mics");
      setInputAnalogVsPDM(EarpieceMixerState::INPUT_ANALOG);
      setAnalogInputSource(EarpieceMixerState::INPUT_PCBMICS);
      setInputConfigButtons();
      setButtonState_frontRearMixer();
      break;
    case 'W':
      Serial.println("EarpieceMixer_F32_UI: Received: Mic jack as mic");
      setInputAnalogVsPDM(EarpieceMixerState::INPUT_ANALOG);
      setAnalogInputSource(EarpieceMixerState::INPUT_MICJACK_MIC);
      setInputConfigButtons();
      setButtonState_frontRearMixer();
      break;
    case 'e':
      Serial.println("EarpieceMixer_F32_UI: Received: Mic jack as line-in");
      setInputAnalogVsPDM(EarpieceMixerState::INPUT_ANALOG);
      setAnalogInputSource(EarpieceMixerState::INPUT_MICJACK_LINEIN);
      setInputConfigButtons();
      setButtonState_frontRearMixer();
      break;     
    case 'E':
      Serial.println("EarpieceMixer_F32_UI: Received: Switch to BT Audio");
      setInputAnalogVsPDM(EarpieceMixerState::INPUT_ANALOG);
      setAnalogInputSource(EarpieceMixerState::INPUT_LINEIN_SE);
      setInputConfigButtons();
      setButtonState_frontRearMixer();
      break;   
    case 'd':
      Serial.println("EarpieceMixer_F32_UI: Received: Listen to PDM mics"); //digital mics
      setInputAnalogVsPDM(EarpieceMixerState::INPUT_PDM);
      setInputConfigButtons();
      setButtonState_frontRearMixer();
      break;  
    case 't':
      Serial.println("EarpieceMixer_F32_UI: Received: Front Mics (if using earpieces)"); 
      configureFrontRearMixer(EarpieceMixerState::MIC_FRONT);
      setButtonState_frontRearMixer();
      break;
    case 'T':
      Serial.println("EarpieceMixer_F32_UI: Received: Mix of Front Mics + Inverted Rear Mics"); 
      configureFrontRearMixer(EarpieceMixerState::MIC_BOTH_INVERTED);
      setButtonState_frontRearMixer();
      break;
     case 'H':
      Serial.println("EarpieceMixer_F32_UI: Received: Rear Mics (if using earpieces)"); 
      configureFrontRearMixer(EarpieceMixerState::MIC_REAR);
      setButtonState_frontRearMixer();
      break;
    case 'i':
      incrementRearMicGain_dB(rearMicGainIncrement_dB);
      setButtonState_rearMic();
      break;
    case 'I':
      incrementRearMicGain_dB(-rearMicGainIncrement_dB);
      setButtonState_rearMic();
      break;
    case 'u':
      { 
        int new_val = state.currentFrontDelay_samps+1;   //increase by 1 
        new_val = setMicDelay_samps(FRONT, new_val);
        setButtonState_micDelay();
      }
      break;
    case 'U':
      { 
        int new_val = state.currentFrontDelay_samps-1;     //reduce by 1 
        new_val = setMicDelay_samps(FRONT, new_val);
        setButtonState_micDelay();
      }
      break;
    case 'y':
      { 
        int new_val = state.currentRearDelay_samps+1;   //increase by 1 
        new_val = setMicDelay_samps(REAR, new_val);
        //Serial.print("Received: Increased rear-mic delay by one sample to ");Serial.println(new_val);
        setButtonState_micDelay();
      }
      break;
    case 'Y':
      { 
        int new_val = state.currentRearDelay_samps-1;     //reduce by 1 
        new_val = setMicDelay_samps(REAR, new_val);
        //Serial.print("Received: Decreased rear-mic delay by one sample to ");Serial.println(new_val);
        setButtonState_micDelay();
      }
      break;
    case 'q':
      configureLeftRightMixer(EarpieceMixerState::INPUTMIX_MUTE);
      Serial.println("EarpieceMixer_F32_UI: Received: Muting audio.");
      setButtonState_inputMixer();
      break;
    case 'Q':
      configureLeftRightMixer(state.input_mixer_nonmute_config);
      setButtonState_inputMixer();
      break;  
    case 'B':
      Serial.println("EarpieceMixer_F32_UI: Received: Input: Mix L+R.");
      configureLeftRightMixer(EarpieceMixerState::INPUTMIX_MONO);
      setButtonState_inputMixer();
      break;  
    case 's':
      configureLeftRightMixer(EarpieceMixerState::INPUTMIX_BOTHLEFT);
      Serial.println("EarpieceMixer_F32_UI: Received: Input: Both Left.");
      setButtonState_inputMixer();
      break;
    case 'S':
      configureLeftRightMixer(EarpieceMixerState::INPUTMIX_BOTHRIGHT);
      Serial.println("EarpieceMixer_F32_UI: Received: Input: Both Right.");
      setButtonState_inputMixer();
      break;   
    default:
      return_val = false;  //we did not process this character
  }
  return return_val;
}

void EarpieceMixer_F32_UI::setFullGUIState(bool activeButtonsOnly) {
  setButtonState_rearMic(activeButtonsOnly);
  setButtonState_micDelay(activeButtonsOnly);
  setButtonState_frontRearMixer(activeButtonsOnly);
  setButtonState_inputMixer(activeButtonsOnly);
  setInputConfigButtons(activeButtonsOnly);
}

void EarpieceMixer_F32_UI::setButtonState_rearMic(bool activeButtonsOnly) {
  setButtonText("rearGain",String(state.rearMicGain_dB,1));
}
void EarpieceMixer_F32_UI::setButtonState_frontRearMixer(bool activeButtonsOnly) {
  bool sendNow = true; //queue them for more efficient transfer at end
  if (!activeButtonsOnly) {
    setButtonState("frontMic",false,sendNow); 
    setButtonState("frontRearMix",false,sendNow);
    setButtonState("rearMic",false,sendNow);
  }
  
  //only activate the active button if we're using the digital mics
  if (state.input_analogVsPDM == EarpieceMixerState::INPUT_PDM) {
    switch (state.input_frontrear_config) {
      case EarpieceMixerState::MIC_FRONT:
        setButtonState("frontMic",true,sendNow);      break;
      case EarpieceMixerState::MIC_BOTH_INVERTED:
        setButtonState("frontRearMix",true,sendNow);  break;
      case EarpieceMixerState::MIC_REAR:
        setButtonState("rearMic",true,sendNow);       break;    
    }
  }
  sendTxBuffer();
}

void EarpieceMixer_F32_UI::setButtonState_micDelay(bool activeButtonsOnly) {
  //Serial.print("setButtonState_frontRearMixer: setting front delaySamps field to ");  Serial.println(String(state.currentFrontDelay_samps));
  setButtonText("delaySampsF",String(state.currentFrontDelay_samps));
  
  //Serial.print("setButtonState_frontRearMixer: setting rear delaySamps field to ");  Serial.println(String(state.currentRearDelay_samps));
  setButtonText("delaySamps",String(state.currentRearDelay_samps));
}

void EarpieceMixer_F32_UI::setButtonState_inputMixer(bool activeButtonsOnly) {
  bool sendNow = true;
    if (!activeButtonsOnly) {
    setButtonState("inp_mute",false,sendNow);
    setButtonState("inp_mono",false,sendNow); 
    setButtonState("inp_micL",false,sendNow);
    setButtonState("inp_micR",false,sendNow);
    //setButtonState("inp_stereo",false);  delay(3); 
    }
     
  switch (state.input_mixer_config) {
    case (EarpieceMixerState::INPUTMIX_STEREO):
      setButtonState("inp_stereo",true,sendNow);  break;
    case (EarpieceMixerState::INPUTMIX_MONO):
      setButtonState("inp_mono",true,sendNow);   break;
    case (EarpieceMixerState::INPUTMIX_MUTE):
      setButtonState("inp_mute",true,sendNow);    break;
    case (EarpieceMixerState::INPUTMIX_BOTHLEFT):
      setButtonState("inp_micL",true,sendNow);    break;
    case (EarpieceMixerState::INPUTMIX_BOTHRIGHT):
      setButtonState("inp_micR",true,sendNow);    break;
  }
  sendTxBuffer(); //send all of the queued up messages now
}


//void EarpieceMixer_F32_UI::setMicConfigButtons(bool activeButtonsOnly) {
//
//  //clear any previous state of the buttons
//  if (!activeButtonsOnly) {
//  setButtonState("micsFront",false); delay(10);
//  setButtonState("micsRear",false); delay(10);
//  setButtonState("micsBoth",false); delay(10);
//  setButtonState("micsBothInv",false); delay(10);
//  setButtonState("micsAIC0",false); delay(10);
//  setButtonState("micsAIC1",false); delay(10);
//  setButtonState("micsBothAIC",false); delay(10);
//  }
//
//  //now, set the one button that should be active
//  int mic_config = myState.input_frontrear_config;
//  if (disableAll == false) {
//    switch (mic_config) {
//      case myState.MIC_FRONT:
//        setButtonState("micsFront",true);
//        break;
//      case myState.MIC_REAR:
//        setButtonState("micsRear",true);
//        break;
//      case myState.MIC_BOTH_INPHASE:
//        setButtonState("micsBoth",true);
//        break;
//      case myState.MIC_BOTH_INVERTED: case myState.MIC_BOTH_INVERTED:
//        setButtonState("micsBothInv",true);
//        break;
//
//    }
//  }
//}


void EarpieceMixer_F32_UI::setInputConfigButtons(bool activeButtonsOnly) {
  //clear out previous state of buttons
  bool sendNow = true; //queue for more efficient transfer 

  if (!activeButtonsOnly) {
    setButtonState("configPDMMic",false,sendNow); 
    setButtonState("configPCBMic",false,sendNow); 
    setButtonState("configMicJack",false,sendNow); 
    setButtonState("configLineJack",false,sendNow);
    setButtonState("configLineSE",false,sendNow);
  }

  //set the new state of the buttons
  if (state.input_analogVsPDM == EarpieceMixerState::INPUT_PDM) {
      setButtonState("configPDMMic",true,sendNow);
  } else {
      switch (state.input_analog_config) {
        case (EarpieceMixerState::INPUT_PCBMICS):
          setButtonState("configPCBMic",true,sendNow);break;
        case (EarpieceMixerState::INPUT_MICJACK_MIC): 
          setButtonState("configMicJack",true,sendNow);break;
        case (EarpieceMixerState::INPUT_LINEIN_SE): 
          setButtonState("configLineSE",true,sendNow);break;
        case (EarpieceMixerState::INPUT_MICJACK_LINEIN): 
          setButtonState("configLineJack",true,sendNow);break;
      }  
  }
}
void EarpieceMixer_F32_UI::setInputGainButtons(bool activeButtonsOnly) {
  setButtonText("inGain",String(state.inputGain_dB,1));
}



TR_Card* EarpieceMixer_F32_UI::addCard_audioSource(TR_Page *page_h) {
  if (page_h == NULL) return NULL;
  TR_Card *card_h = page_h->addCard("Audio Source");
  if (card_h == NULL) return NULL;
  String prefix = String(quadchar_start_char)+String(ID_char)+String("x");
  card_h->addButton(F("Digital Earpieces"),       prefix+"d", F("configPDMMic"),   12); //label, command, id, width
  card_h->addButton(F("Analog: PCB Mics"),        prefix+"w", F("configPCBMic"),   12); //label, command, id, width
  card_h->addButton(F("Analog: Mic Jack (Mic)"),  prefix+"W", F("configMicJack"),  12); //label, command, id, width
  card_h->addButton(F("Analog: Mic Jack (Line)"), prefix+"e", F("configLineJack"), 12); //label, command, id, width
  card_h->addButton(F("Analog: BT Audio"),        prefix+"E", F("configLineSE"),   12); //label, command, id, width
  return card_h;
}

TR_Card* EarpieceMixer_F32_UI::addCard_frontRearMics(TR_Page *page_h) {
  if (page_h == NULL) return NULL;
  TR_Card *card_h = page_h->addCard("Front or Rear Mics");
  if (card_h == NULL) return NULL;
  String prefix = String(quadchar_start_char)+String(ID_char)+String("x");
  card_h->addButton("Front",    prefix+"t", "frontMic",     4); //label, command, id, width
  card_h->addButton("Mix(F-R)", prefix+"T", "frontRearMix", 4); //label, command, id, width
  card_h->addButton("Rear",     prefix+"H", "rearMic",      4); //label, command, id, width
  return card_h;   
}
TR_Card* EarpieceMixer_F32_UI::addCard_rearMicGain(TR_Page *page_h) {
  if (page_h == NULL) return NULL;
  TR_Card *card_h = page_h->addCard("Rear Mic Gain (dB)");
  if (card_h == NULL) return NULL;
  String prefix = String(quadchar_start_char)+String(ID_char)+String("x");
  card_h->addButton("-", prefix+"I", "",         4); //label, command, id, width
  card_h->addButton("" , "" ,        "rearGain", 4); //label, command, id, width
  card_h->addButton("+", prefix+"i", "",         4); //label, command, id, width
  return card_h;   
}
TR_Card* EarpieceMixer_F32_UI::addCard_frontMicDelay(TR_Page *page_h) {
  if (page_h == NULL) return NULL;
  TR_Card *card_h = page_h->addCard("Front Mic Delay (samples)");
  if (card_h == NULL) return NULL;
    String prefix = String(quadchar_start_char)+String(ID_char)+String("x");
  card_h->addButton("-", prefix+"U", "",            4); //label, command, id, width
  card_h->addButton("" , "",         "delaySampsF", 4); //label, command, id, width
  card_h->addButton("+", prefix+"u", "",            4); //label, command, id, width
  return card_h;   
}
TR_Card* EarpieceMixer_F32_UI::addCard_rearMicDelay(TR_Page *page_h) {
  if (page_h == NULL) return NULL;
  TR_Card *card_h = page_h->addCard("Rear Mic Delay (samples)");
  if (card_h == NULL) return NULL;
    String prefix = String(quadchar_start_char)+String(ID_char)+String("x");
  card_h->addButton("-", prefix+"Y", "",           4); //label, command, id, width
  card_h->addButton("" , "",         "delaySamps", 4); //label, command, id, width
  card_h->addButton("+", prefix+"y", "",           4); //label, command, id, width
  return card_h;   
}
TR_Page* EarpieceMixer_F32_UI::addPage_digitalEarpieces(TympanRemoteFormatter *gui) {
  if (gui == NULL) return NULL;
  TR_Page *page_h = gui->addPage("Digital Earpieces");
  if (page_h == NULL) return NULL;
  addCard_frontRearMics(page_h);
  addCard_frontMicDelay(page_h);
  addCard_rearMicDelay(page_h);
  addCard_rearMicGain(page_h);
  return page_h;
}
    
