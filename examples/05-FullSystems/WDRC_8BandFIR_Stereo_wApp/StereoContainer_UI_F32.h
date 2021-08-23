
#ifndef _StereoContainer_UI_F32
#define _StereoContainer_UI_F32

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

class StereoContainer_UI_F32 : public SerialManager_UI {
  public:
    StereoContainer_UI_F32(void) : SerialManager_UI() {};

    enum channel {LEFT=0, RIGHT=1};

    void add_item_pair(SerialManager_UI *left_item, SerialManager_UI *right_item);

    virtual int get_cur_channel(void) { return state.cur_channel; }
    virtual int set_cur_channel(int val) { val = max(LEFT,min(RIGHT, val)); return state.cur_channel = val; }
    
    std::vector<SerialManager_UI *> items_L;  //this really only needs to be [max_n_filters-1] in length, but we'll generally allocate [max_n_filters] just to avoid mistaken overruns
    std::vector<SerialManager_UI *> items_R;  //this really only needs to be [max_n_filters-1] in length, but we'll generally allocate [max_n_filters] just to avoid mistaken overruns

    StereoContainerState state;

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

void StereoContainer_UI_F32::add_item_pair(SerialManager_UI *left_item, SerialManager_UI *right_item) {
  items_L.push_back(left_item);
  items_R.push_back(right_item); 
}

void StereoContainer_UI_F32::printHelp(void) {
  for (int i=0; (unsigned int)i < items_L.size(); i++) items_L[i]->printHelp();
}

bool StereoContainer_UI_F32::processCharacterTriple(char item_ID_char, char chan_char, char data_char) {

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

bool StereoContainer_UI_F32::processCharacterTriple_self(char item_ID_char, char chan_char, char data_char) {
  //make sure that the given ID character matches
  if (item_ID_char != getIDchar()) return false;

  //ignore the chan_char

  //process the data character
  return processCharacter(data_char);
}

bool StereoContainer_UI_F32::processCharacterTriple_items(char item_ID_char, char chan_char, char data_char, int item_index) {
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

bool StereoContainer_UI_F32::processCharacter(char c) {
  bool ret_val = true;
  switch (c) {
    case '0':  //zero
      set_cur_channel(c);
      setFullGUIState();
      break;
    case '1':  //one
      set_cur_channel(c);
      setFullGUIState();
      break;
    default:
      ret_val = false;
      break;
  }
  return ret_val;
}
    

void StereoContainer_UI_F32::setFullGUIState(bool activeButtonsOnly) {
  updateCard_chooseChan(activeButtonsOnly);
  
  if (get_cur_channel() == RIGHT) {
    for (int i=0; (unsigned int)i < items_R.size(); i++) items_R[i]->setFullGUIState();    
  } else {
    for (int i=0; (unsigned int)i < items_L.size(); i++) items_L[i]->setFullGUIState();
  }
}

TR_Card* StereoContainer_UI_F32::addCard_chooseChan(TR_Page *page_h) {
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

void StereoContainer_UI_F32::updateCard_chooseChan(bool activeButtonsOnly) {
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
