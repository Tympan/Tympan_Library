

#include "SerialManager_UI.h"
#include "SerialManagerBase.h"

char SerialManager_UI::next_ID_char = '0';  //start at zero and increment through the ASCII table
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