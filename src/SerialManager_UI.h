
#ifndef SerialManager_UI_h
#define SerialManager_UI_h

#include <Arduino.h> //for String
#include <TympanRemoteFormatter.h> //for TR_Page
//#include "SerialManagerBase.h"
class SerialManagerBase;  //forward declaration

class SerialManager_UI {
  public:
    SerialManager_UI(void) { 
      ID_char = next_ID_char++;
    };
	SerialManager_UI(SerialManagerBase  *_sm) {
		ID_char = next_ID_char++;
		setSerialManager(_sm);
	}

    // ///////// these are the methods that you must implement when you inheret
    virtual void printHelp(void) {};        				   //default is to print nothing
    virtual bool processCharacter(char c) { return false; } ;  //default is to do nothing
    virtual bool processCharacterTriple(char mode_char, char chan_char, char data_char) { return false; }; //default is to do nothing
    virtual void setFullGUIState(bool activeButtonsOnly=false) {};  //default is to do nothing
    // ///////////////////////
	
	//create the button sets for the TympanRemote's GUI, which you can override
	virtual TR_Page* addPage_default(TympanRemoteFormatter *gui) { return NULL; }

	// predefined helper functions, which you can override
	virtual String getPrefix(void) { return String(quadchar_start_char) + String(ID_char) + String("x"); }  //your class can use any and every String-able character in place of "x"...so, you class can have *a lot* of commands
	
    //attach the SerialManager
    void setSerialManager(SerialManagerBase *_sm);
    SerialManagerBase *getSerialManager(void);
    
  protected:
    char ID_char;                    //see SerialManager_UI.cpp for where it gets initializedd
    static char quadchar_start_char; //see SerialManager_UI.cpp for where it gets initializedd
    SerialManagerBase *sm = NULL;
    virtual void setButtonState(String btnId, bool newState, bool sendNow = true);
    virtual void setButtonText(String btnId, String text);
    virtual void sendTxBuffer(void);
  private:
    static char next_ID_char;
};

#endif