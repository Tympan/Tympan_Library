
#ifndef SerialManager_UI_h
#define SerialManager_UI_h

#include <Arduino.h> //for String
//#include "SerialManagerBase.h"
class SerialManagerBase;  //forward declaration

class SerialManager_UI {
  public:
    SerialManager_UI(void) { 
      ID_char = next_ID_char++;
    };

    // ///////// these are the methods that you must implement when you inheret
    virtual void printHelp(void) {};        				   //default is to print nothing
    virtual bool processCharacter(char c) { return false; } ;  //default is to do nothing
    virtual bool processCharacterTriple(char mode_char, char chan_char, char data_char) { return false; }; //default is to do nothing
    virtual void setFullGUIState(bool activeButtonsOnly=false) {};  //default is to do nothing
    // ///////////////////////

    //attach the SerialManager
    void setSerialManager(SerialManagerBase *_sm);
    SerialManagerBase *getSerialManager(void);
    
  protected:
    char ID_char;
    static char quadchar_start_char;
    SerialManagerBase *sm = NULL;
    virtual void setButtonState(String btnId, bool newState, bool sendNow = true);
    virtual void setButtonText(String btnId, String text);
    virtual void sendTxBuffer(void);
  private:
    static char next_ID_char;
};

#endif