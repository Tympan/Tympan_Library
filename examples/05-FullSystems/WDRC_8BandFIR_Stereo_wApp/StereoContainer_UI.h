
#ifndef _StereoContainer_UI_h
#define _StereoContainer_UI_h

#include <Arduino.h>
//#include <AudioStream_F32.h>
#include <SerialManager_UI.h>       //from Tympan_Library
#include <TympanRemoteFormatter.h>      //from Tympan_Library
#include <vector>

class StereoContainerState {
  public:
    StereoContainerState(void) {};
    int cur_channel = 0; 
        
  protected:

};

class StereoContainer_UI : public SerialManager_UI {
  public:
    StereoContainer_UI(void) : SerialManager_UI() {};

    enum channel {LEFT=0, RIGHT=1};


     void add_item_pair(SerialManager_UI *left_item, SerialManager_UI *right_item);

    virtual int get_cur_channel(void) { return state.cur_channel; }
    virtual int set_cur_channel(int val) { val = max(LEFT,min(RIGHT, val)); return state.cur_channel = val; }
    
    std::vector<SerialManager_UI *> items_L;  //this really only needs to be [max_n_filters-1] in length, but we'll generally allocate [max_n_filters] just to avoid mistaken overruns
    std::vector<SerialManager_UI *> items_R;  //this really only needs to be [max_n_filters-1] in length, but we'll generally allocate [max_n_filters] just to avoid mistaken overruns

    StereoContainerState state;

    // /////////////////////////////////////// Overload methods from SerialManager_UI
    void setSerialManager(SerialManagerBase *_sm);

    // ///////////////////////////////////////  Required methods because of SerialManager_UI
    virtual void printHelp(void);
    virtual bool processCharacterTriple(char mode_char, char chan_char, char data_char);
    virtual void setFullGUIState(bool activeButtonsOnly = false); 
    // ///////// end of required methods
    virtual bool processCharacter(char c);
    

    // /////////////////////////////////////// Tympan Remote Formatter Methods

    TR_Card* addCard_chooseChan(TR_Page *page_h); //left or right
    void updateCard_chooseChan(bool activeButtonsOnly = false);
    //TR_Page* addPage_default(TympanRemoteFormatter *gui) {};
    
  protected:  
    virtual bool processCharacterTriple_self(char mode_char, char chan_char, char data_char);
    virtual bool processCharacterTriple_items(char item_ID_char, char chan_char, char data_char, int item_index);

  
};

void StereoContainer_UI::setSerialManager(SerialManagerBase *_sm) {
  //normally, "setSerialManager" is called automatically when an UI instance is added to the 
  //SerialManager via SerialManagerBase::add_UI_element().  But, by using this container class,
  //the components within the container are never handed to SerialManagerBase::add_UI_element().
  //Therefore, we must do it ourselves here.
  
  for (int i=0; (unsigned int)i<items_L.size(); i++) {
    items_L[i]->setSerialManager(_sm);  //from SerialManager_UI.
    items_R[i]->setSerialManager(_sm);
  }

  SerialManager_UI::setSerialManager(_sm);
}

void StereoContainer_UI::add_item_pair(SerialManager_UI *left_item, SerialManager_UI *right_item) {
  items_L.push_back(left_item);
  items_R.push_back(right_item); 

  //if items are added after the setSerialManager() has been called, we should add the link to
  //serialManager now.
  SerialManagerBase *_sm = getSerialManager();  //from SerialManager_UI.h
  if (_sm != NULL) {
    left_item->setSerialManager(_sm);
    right_item->setSerialManager(_sm);
  }
}

void StereoContainer_UI::printHelp(void) {
  for (int i=0; (unsigned int)i < items_L.size(); i++) items_L[i]->printHelp();
}

bool StereoContainer_UI::processCharacterTriple(char item_ID_char, char chan_char, char data_char) {

  //check the item_ID_char to see what (if anything) it matches
  if (getIDchar() == item_ID_char) { //compare to this container's own ID character
    return processCharacterTriple_self(item_ID_char, chan_char, data_char);
  } else {    
    //compare against each of the container's items
    for (int i=0; (unsigned int)i<items_L.size(); i++) {
      if (items_L[i]->getIDchar() == item_ID_char) {
        return processCharacterTriple_items(item_ID_char, chan_char, data_char, i);
      }
    }
  }
  return false;
}

bool StereoContainer_UI::processCharacterTriple_self(char item_ID_char, char chan_char, char data_char) {
  //make sure that the given ID character matches
  if (item_ID_char != getIDchar()) return false;

  //ignore the chan_char

  //process the data character
  return processCharacter(data_char);
}

bool StereoContainer_UI::processCharacterTriple_items(char item_ID_char, char chan_char, char data_char, int item_index) {
  if (get_cur_channel() == LEFT) {
      //the item_ID_char already is the left side
      return items_L[item_index]->processCharacterTriple(item_ID_char, chan_char, data_char);
  } else { //current channel is right side
      //change the item_ID_char to match the right side
      item_ID_char =items_R[item_index]->getIDchar();
      return items_R[item_index]->processCharacterTriple(item_ID_char, chan_char, data_char);
  }
  return false;
}

bool StereoContainer_UI::processCharacter(char c) {
  bool ret_val = true;
  switch (c) {
    case '0':  //zero
      Serial.println(F("StereoContainer_UI: changing to channel 0 (LEFT)..."));
      set_cur_channel(0);
      setFullGUIState();
      break;
    case '1':  //one
      Serial.println(F("StereoContainer_UI: changing to channel 1 (RIGHT)..."));
      set_cur_channel(1);
      setFullGUIState();
      break;
    default:
      ret_val = false;
      break;
  }
  return ret_val;
}
    

void StereoContainer_UI::setFullGUIState(bool activeButtonsOnly) {
  updateCard_chooseChan(activeButtonsOnly);
  if (get_cur_channel() == LEFT) {
    for (int i=0; (unsigned int)i < items_L.size(); i++) {
      Serial.println("StereoContainer_UI: setFullGUIState: updating Left item " + String(i));
      items_L[i]->setFullGUIState();    
    }
  } else {
    for (int i=0; (unsigned int)i < items_R.size(); i++) {
      Serial.println("StereoContainer_UI: setFullGUIState: updating Right item " + String(i));
      items_R[i]->setFullGUIState();
    }
  }
}

TR_Card* StereoContainer_UI::addCard_chooseChan(TR_Page *page_h) {
  if (page_h == NULL) return NULL;
  TR_Card *card_h = page_h->addCard("Choose Ear");
  if (card_h == NULL) return NULL;

  String prefix = getPrefix();     //3 character code.  getPrefix() is here in SerialManager_UI.h, unless it is over-ridden in the child class somewhere
  //prefix[2] = persistCharTrigger;  //replace the "channel" with the persistent-mode-triggering character
  String ID = String(ID_char);
  
  card_h->addButton("Left",   prefix+"0", ID+"chan0",   6); //label, command, id, width...this is the minus button
  card_h->addButton("Right",  prefix+"1", ID+"chan1",   6); //label, command, id, width...this is the minus button

  return card_h; 
}

void StereoContainer_UI::updateCard_chooseChan(bool activeButtonsOnly) {
  String ID = String(ID_char);
  
  //loop over all buttons and send the "off" messages
  if (activeButtonsOnly == false) {
    for (int Ichan=LEFT; Ichan<=RIGHT; Ichan++) {
      if (Ichan != get_cur_channel()) setButtonState(ID+"chan"+String(Ichan),false);
    }
  }

  //send the "on" message
  setButtonState(ID+"chan"+String(get_cur_channel()),true);
}

#endif
