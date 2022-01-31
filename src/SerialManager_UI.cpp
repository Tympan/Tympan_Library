

#include "SerialManager_UI.h"
#include "SerialManagerBase.h"

char SerialManager_UI::next_ID_char = 'A';  //start at this value and then increment through the ASCII table
char SerialManager_UI::quadchar_start_char = QUADCHAR_START_CHAR;


void SerialManager_UI::setSerialManager(SerialManagerBase *_sm) { 
	sm = _sm; 
};

SerialManagerBase* SerialManager_UI::getSerialManager(void) { 
	return sm; 
};

void SerialManager_UI::setButtonState(String btnId, bool newState, bool sendNow) { 
	if (sm) sm->setButtonState(btnId, newState, sendNow);
}

void SerialManager_UI::setButtonText(String btnId, String text) {  
	if (sm) sm->setButtonText(btnId,text); 
};

void SerialManager_UI::sendTxBuffer(void) { 
	if (sm) sm->sendTxBuffer(); 
};



// //////////////////////////////////////////////////////////////////

// here is a method to create the very-common card (button group) to create display a parameter value
// and to adjust its value with a plus and minus button.  Very common!
TR_Card* SerialManager_UI::addCardPreset_UpDown(TR_Page *page_h, const String card_title, const String field_name, const String down_cmd, const String up_cmd) {
	if (page_h == NULL) return NULL;
	TR_Card *card_h = page_h->addCard(card_title);
	if (card_h == NULL) return NULL;
	
	addButtons_presetUpDown(card_h, field_name, down_cmd, up_cmd);
	return card_h;
}
void SerialManager_UI::addButtons_presetUpDown(TR_Card *card_h, const String field_name, const String down_cmd, const String up_cmd) {
	if (card_h == NULL) return;

	String prefix = getPrefix();    //3 character code.  getPrefix() is here in SerialManager_UI.h, unless it is over-ridden in the child class somewhere
	//String field_name1 = ID_char + field_name;  //prepend the field name with the unique ID_char to that it is only associated with a particular instance of the class
	String field_name1 = ID_char_fn + field_name;  //prepend the field name with the unique ID_char to that it is only associated with a particular instance of the class

	card_h->addButton("-",   prefix+down_cmd, "",          4); //label, command, id, width...this is the minus button
	card_h->addButton("",    "",              field_name1, 4); //label, command, id, width...this text to display the value 
	card_h->addButton("+",   prefix+up_cmd,   "",          4); //label, command, id, width...this is the plus button
}


// here is a multi channel version of the up-down button display
TR_Card* SerialManager_UI::addCardPreset_UpDown_multiChan(TR_Page *page_h, const String card_title, const String field_name, const String down_cmd, const String up_cmd, int n_chan) {
	if (page_h == NULL) return NULL;
	TR_Card *card_h = page_h->addCard(card_title);
	if (card_h == NULL) return NULL;

	addButtons_presetUpDown_multiChan(card_h, field_name, down_cmd, up_cmd, n_chan);
	return card_h;
}
void SerialManager_UI::addButtons_presetUpDown_multiChan(TR_Card *card_h, const String field_name, const String down_cmd, const String up_cmd, int n_chan) {
	if (card_h == NULL) return;

	String prefix = getPrefix();   //3 character code.  getPrefix() is here in SerialManager_UI.h, unless it is over-ridden in the child class somewhere

	if ((n_chan < 1) || (n_chan > n_charMap)) return;

	for (int i=0; i<n_chan; i++) {
		//String fn_wChan = ID_char + field_name + String(i);     //channel number as a character (well, as a string)
		String fn_wChan = ID_char_fn + field_name + String(i);     //channel number as a character (well, as a string)
		//String pre_wChan = String(prefix[0])+String(prefix[1])+String(i);  //drop the 3rd character of the prefix and replace with chan number			
		String pre_wChan = String(prefix[0])+String(prefix[1])+charMapUp[i]; //drop the 3rd character of the prefix and replace with chan number			
		String cp1 = String(i+1); //for display to a human, who counts from 1 (not zero)
		
		card_h->addButton(cp1,   "",                 "",       2);  //label, command, id, width...this is the minus button
		card_h->addButton("-",   pre_wChan+down_cmd, "",       3);  //label, command, id, width...this is the minus button
		card_h->addButton("",    "",                 fn_wChan, 4);  //label, command, id, width...this text to display the value 
		card_h->addButton("+",   pre_wChan+up_cmd,   "",       3);  //label, command, id, width...this is the plus button
	}
}


//Method to update the text associated the "button" used to display a parameter's values.  This updates
//the button that was named by the convention used by "addCardPreset_UpDown()" so it pairs nicely with
//buttons created by that method.
void SerialManager_UI::updateCardPreset_UpDown(const String field_name,const String new_text) {
	//String field_name1 = ID_char + field_name;  //prepend the unique ID_char so that the text goes to the correct button and not one similarly named
	String field_name1 = ID_char_fn + field_name;  //prepend the unique ID_char so that the text goes to the correct button and not one similarly named
	setButtonText(field_name1, new_text);
} 
void SerialManager_UI::updateCardPreset_UpDown(const String field_name,const String new_text, int Ichan) {
	//String field_name1 = ID_char + field_name + String(Ichan);  //prepend the unique ID_char so that the text goes to the correct button and not one similarly named
	String field_name1 = ID_char_fn + field_name + String(Ichan);  //prepend the unique ID_char so that the text goes to the correct button and not one similarly named
	setButtonText(field_name1, new_text);
} 
