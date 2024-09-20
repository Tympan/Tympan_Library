
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
			ID_char_fn = ID_char;
    };
		SerialManager_UI(SerialManagerBase  *_sm) {
			ID_char = next_ID_char++;
			ID_char_fn = ID_char;
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
		virtual String getPrefix(void) { return String(quadchar_start_char) + String(ID_char) + prefix_globalChar; }  //your class can use any and every String-able character in place of "x"...so, you class can have *a lot* of commands
		virtual char get_ID_char_fn(void) { return ID_char_fn; }
		virtual char set_ID_char_fn(char c) { return ID_char_fn = c; }
		
		// here is a method to create the very-common card (button group) to create display a parameter value
		// and to adjust its value with a plus and minus button.  Very common!
		virtual TR_Card* addCardPreset_UpDown(TR_Page *page_h, const String card_title, const String field_name, const String down_cmd, const String up_cmd);
		virtual void addButtons_presetUpDown(TR_Card *card_h, const String field_name, const String down_cmd, const String up_cmd);
		
		// here is a multi channel version of the up-down button display
		virtual TR_Card* addCardPreset_UpDown_multiChan(TR_Page *page_h, const String card_title, const String field_name, const String down_cmd, const String up_cmd, int n_chan);
		virtual void addButtons_presetUpDown_multiChan(TR_Card *card_h, const String field_name, const String down_cmd, const String up_cmd, int n_chan);


		//Method to update the text associated the "button" used to display a parameter's values.  This updates
		//the button that was named by the convention used by "addCardPreset_UpDown()" so it pairs nicely with
		//buttons created by that method.
		virtual void updateCardPreset_UpDown(const String field_name,const String new_text);
		virtual void updateCardPreset_UpDown(const String field_name,const String new_text, int Ichan);

    //attach the SerialManager
    virtual void setSerialManager(SerialManagerBase *_sm);
    virtual SerialManagerBase *getSerialManager(void);
	
		//useful value
		const int capOffset = 65 - 97; //given lower case, add this value to get the upper case
		
		//adjust the prefix characters
		virtual char get_prefix_globalChar(void) { return prefix_globalChar; };
		virtual char set_prefix_globalChar(char c) { prefix_globalChar = c;  return get_prefix_globalChar(); }


  protected:
		char ID_char;                    //initialized in constructor.  
		char ID_char_fn;				 //initialized in constructor.  this normally the same as ID_char, but can be made different if you want to overwrite someone else's fieldnames in the GUI widgets
		static char quadchar_start_char; //see SerialManager_UI.cpp for where it gets initialized
		SerialManagerBase *sm = NULL;
		virtual void setButtonState(String btnId, bool newState, bool sendNow = true);
		virtual void setButtonText(String btnId, String text);
		virtual void sendTxBuffer(void);
		char prefix_globalChar = 'x';  //by default, this character is used as the "channel" character. For most of your Audio classes, this won't matter at all.

		#define SERIALUI_N_CHARMAP (10+26+1)
		const int n_charMap = SERIALUI_N_CHARMAP;
		char charMapUp[SERIALUI_N_CHARMAP]   = "0123456789abcdefghijklmnopqrstuvwxyz"; //characters for raising
		char charMapDown[SERIALUI_N_CHARMAP] = ")!@#$%^&*(ABCDEFGHIJKLMNOPQRSTUVWXYZ"; //characters for lowering

	
  private:
    static char next_ID_char;      //see SerialManager_UI.cpp for where it gets initialized
};

#endif