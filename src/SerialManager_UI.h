
#ifndef SerialManager_UI_h
#define SerialManager_UI_h

#include <Arduino.h> //for String
#include <TympanRemoteFormatter.h> //for TR_Page
//#include "SerialManagerBase.h"
#include <TympanRemoteFormatter.h>

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
	virtual char getIDchar() { return ID_char; }
	virtual String getPrefix(void) { return String(quadchar_start_char) + String(ID_char) + prefix_placeholder_char; }  //your class can use any and every String-able character in place of "x"...so, you class can have *a lot* of commands
	
	// here is a method to create the very-common card (button group) to create display a parameter value
	// and to adjust its value with a plus and minus button.  Very common!
	virtual TR_Card* addCardPreset_UpDown(TR_Page *page_h, const String card_title, const String field_name, const String down_cmd, const String up_cmd) {
		if (page_h == NULL) return NULL;
		TR_Card *card_h = page_h->addCard(card_title);
		if (card_h == NULL) return NULL;

		String prefix = getPrefix();    //3 character code.  getPrefix() is here in SerialManager_UI.h, unless it is over-ridden in the child class somewhere
		String field_name1 = ID_char + field_name;  //prepend the field name with the unique ID_char to that it is only associated with a particular instance of the class

		card_h->addButton("-",   prefix+down_cmd, "",          4); //label, command, id, width...this is the minus button
		card_h->addButton("",    "",              field_name1, 4); //label, command, id, width...this text to display the value 
		card_h->addButton("+",   prefix+up_cmd,   "",          4); //label, command, id, width...this is the plus button

		return card_h;
	}
	// here is a multi channel version of the up-down button display
	virtual TR_Card* addCardPreset_UpDown_multiChan(TR_Page *page_h, const String card_title, const String field_name, const String down_cmd, const String up_cmd, int n_chan) {
		if (page_h == NULL) return NULL;
		TR_Card *card_h = page_h->addCard(card_title);
		if (card_h == NULL) return NULL;

		String prefix = getPrefix();   //3 character code.  getPrefix() is here in SerialManager_UI.h, unless it is over-ridden in the child class somewhere

		for (int i=0; i<n_chan; i++) {
			String fn_wChan = ID_char + field_name + String(i);     //channel number as a character (well, as a string)
			String pre_wChan = String(prefix[0])+String(prefix[1])+String(i); //drop the 3rd character of the prefix and replace with chan number			
			String cp1 = String(i+1); //for display to a human, who counts from 1 (not zero)
			
			card_h->addButton(cp1,   "",                 "",       2);  //label, command, id, width...this is the minus button
			card_h->addButton("-",   pre_wChan+down_cmd, "",       3);  //label, command, id, width...this is the minus button
			card_h->addButton("",    "",                 fn_wChan, 4);  //label, command, id, width...this text to display the value 
			card_h->addButton("+",   pre_wChan+up_cmd,   "",       3);  //label, command, id, width...this is the plus button
		}
		return card_h;
	}


	//Method to update the text associated the "button" used to display a parameter's values.  This updates
	//the button that was named by the convention used by "addCardPreset_UpDown()" so it pairs nicely with
	//buttons created by that method.
	void updateCardPreset_UpDown(const String field_name,const String new_text) {
		String field_name1 = ID_char + field_name;  //prepend the unique ID_char so that the text goes to the correct button and not one similarly named
		setButtonText(field_name1, new_text);
	} 
	void updateCardPreset_UpDown(const String field_name,const String new_text, int Ichan) {
		String field_name1 = ID_char + field_name + String(Ichan);  //prepend the unique ID_char so that the text goes to the correct button and not one similarly named
		setButtonText(field_name1, new_text);
	} 

	
    //attach the SerialManager
    void setSerialManager(SerialManagerBase *_sm);
    SerialManagerBase *getSerialManager(void);
	
	//useful value
	const int capOffset = 41 - 61; //given lower case, add this value to get the upper case

  protected:
    char ID_char;                    //see SerialManager_UI.cpp for where it gets initializedd
    static char quadchar_start_char; //see SerialManager_UI.cpp for where it gets initializedd
    SerialManagerBase *sm = NULL;
    virtual void setButtonState(String btnId, bool newState, bool sendNow = true);
    virtual void setButtonText(String btnId, String text);
    virtual void sendTxBuffer(void);
	char prefix_placeholder_char = 'x';
  private:
    static char next_ID_char;
};

#endif