
#include "SerialManagerBase.h"
#include "SerialManager_UI.h"

//Register a UI element with the SerialManager
SerialManager_UI* SerialManagerBase::add_UI_element(SerialManager_UI *ptr) {
	if (ptr == NULL) return NULL;
	
	//add the UI element to its tracking of all UI elements
	UI_element_ptr.push_back(ptr);
	
	//tell the UI element that this is the SerialManager for it to call
	ptr->setSerialManager(this);
	
	return ptr;
}

void SerialManagerBase::printHelp(void) {
	if (UI_element_ptr.size() == 0) return;   //if there are no registered UI elements, return

	
	//Loop over each registered UI element and call its printHelp() method.
	for (int i=0; (unsigned int)i < UI_element_ptr.size(); i++) {		
		if (UI_element_ptr[i]) { //make sure it isn't NULL
			UI_element_ptr[i]->printHelp();
		}
	}
}

//switch yard to determine the desired action
void SerialManagerBase::respondToByte(char c) {
  switch (serial_read_state) {
    case SINGLE_CHAR:
      if (c == DATASTREAM_START_CHAR) {
        Serial.println("SerialManagerBase: RespondToByte: Start data stream.");
        // Start a data stream:
        serial_read_state = STREAM_LENGTH;
        stream_chars_received = 0;
      } else if (c == QUADCHAR_START_CHAR) {
        serial_read_state = QUAD_CHAR_1;
      } else {
        if ((c != '\r') && (c != '\n')) {
          //Serial.print("SerialManagerBase: RespondToByte: Processing character "); Serial.println(c);
          processCharacter(c);
        }
      }
      break;
    case STREAM_LENGTH:
      if (c == DATASTREAM_SEPARATOR) {
        // Get the datastream length from stream_data.
        int* stream_data_as_int = reinterpret_cast<int*>(stream_data);
        stream_length = *stream_data_as_int;
        serial_read_state = STREAM_DATA;
        stream_chars_received = 0;
        Serial.print("SerialManagerBase: RespondToByte: Stream length = ");
        Serial.println(stream_length);
      } else {
        stream_data[stream_chars_received] = c;
        stream_chars_received++;
      }
      break;
    case STREAM_DATA:
      if (stream_chars_received < stream_length) {
        stream_data[stream_chars_received] = c;
        stream_chars_received++;        
      } else {
        if (c == DATASTREAM_END_CHAR) {
          // successfully read the stream
          Serial.println(F("SerialManagerBase: RespondToByte: Time to process stream!"));
          processStream();
        } else {
          Serial.print(F("SerialManagerBase: RespondToByte: ERROR: Expected string terminator "));
          Serial.print(DATASTREAM_END_CHAR, HEX);
          Serial.print("; found ");
          Serial.print(c,HEX);
          Serial.println(" instead.");
        }
        serial_read_state = SINGLE_CHAR;
        stream_chars_received = 0;
      }
      break;
    case QUAD_CHAR_1:
      if ((c == '\n') || (c == '\r')) { serial_read_state = SINGLE_CHAR; return; } // abort
      if (c == DATASTREAM_START_CHAR) { Serial.println("SerialManagerBase: RespondToByte: Start data stream."); serial_read_state = STREAM_LENGTH; stream_chars_received = 0; return; }//abort into stream mode
      mode_char = c; chan_char = (char)NULL; data_char = (char)NULL;
      serial_read_state = QUAD_CHAR_2;
      break;
    case QUAD_CHAR_2:
      if ((c == '\n') || (c == '\r')) { serial_read_state = SINGLE_CHAR; return; } // abort
      if (c == DATASTREAM_START_CHAR) { Serial.println("SerialManagerBase: RespondToByte: Start data stream."); serial_read_state = STREAM_LENGTH; stream_chars_received = 0; return;} //abort into stream mode
      chan_char = c;
      serial_read_state = QUAD_CHAR_3;
      break;
    case QUAD_CHAR_3:
      if ((c == '\n') || (c == '\r')) { serial_read_state = SINGLE_CHAR; return; } // abort
      if (c == DATASTREAM_START_CHAR) { Serial.println("SerialManagerBase: RespondToByte: Start data stream."); serial_read_state = STREAM_LENGTH; stream_chars_received = 0; return; }//abort into stream mode
      data_char = c;
      interpretQuadChar(mode_char, chan_char, data_char);
      serial_read_state = SINGLE_CHAR;
      break; 
  }
}


bool SerialManagerBase::processCharacter(char c) {
	bool ret_val = false;
	if (UI_element_ptr.size() == 0) return ret_val;   //if there are no registered UI elements, return
	
	// Loop over each registered UI element and try its processCharacter() method.
	// Their processCharacter() will return TRUE if the character was recognized.
	// If the TRUE is recevied, processing stops.  If a FALSE is received, this routine
	// continues on to the next registered UI element.
	for (int i=0; (unsigned int)i < UI_element_ptr.size() ; i++) {
		if (UI_element_ptr[i]) { //make sure it isn't NULL
			ret_val = UI_element_ptr[i]->processCharacter(c);
			if (ret_val) return ret_val;
		}
	}
	return ret_val;
}


void SerialManagerBase::processStream(void) {
  int idx = 0;
  String streamType;
  int tmpInt;
  float tmpFloat;
  
  while (stream_data[idx] != DATASTREAM_SEPARATOR) {
    streamType.append(stream_data[idx]);
    idx++;
  }
  idx++; // move past the datastream separator

  //Serial.print("Received stream: ");
  //Serial.print(stream_data);

  if (streamType == "gha") {    
    Serial.println("Stream is of type 'gha'.");
    //interpretStreamGHA(idx);
  } else if (streamType == "dsl") {
    Serial.println("Stream is of type 'dsl'.");
    //interpretStreamDSL(idx);
  } else if (streamType == "afc") {
    Serial.println("Stream is of type 'afc'.");
    //interpretStreamAFC(idx);
  } else if (streamType == "test") {    
    Serial.println("Stream is of type 'test'.");
    tmpInt = *((int*)(stream_data+idx)); idx = idx+4;
    Serial.print("int is "); Serial.println(tmpInt);
    tmpFloat = *((float*)(stream_data+idx)); idx = idx+4;
    Serial.print("float is "); Serial.println(tmpFloat);
  } else {
    Serial.print("Unknown stream type: ");
    Serial.println(streamType);
  }
}


bool SerialManagerBase::interpretQuadChar(char mode_char, char chan_char, char data_char) {
	bool ret_val = false;
	if (UI_element_ptr.size() == 0) return ret_val;
	
	// Loop over each registered UI element and try its processCharacterTriple() method.
	// Their processCharacterTriple() will return TRUE if the character was recognized.
	// If the TRUE is recevied, processing stops.  If a FALSE is received, this routine
	// continues on to the next registered UI element.
	for (int i=0; (unsigned int)i < UI_element_ptr.size() ; i++) {
		if (UI_element_ptr[i]) { //make sure it isn't NULL
			ret_val = UI_element_ptr[i]->processCharacterTriple(mode_char, chan_char, data_char);
			if (ret_val) return ret_val;
		}
	}
	return ret_val;
}

void SerialManagerBase::setFullGUIState(bool activeButtonsOnly) {
	if (UI_element_ptr.size() == 0) return;	
	
	// Loop over each registered UI element and execute its setFullGUIState method.
	for (int i=0; (unsigned int)i < UI_element_ptr.size() ; i++) {
		if (UI_element_ptr[i]) { //make sure it isn't NULL
			UI_element_ptr[i]->setFullGUIState(activeButtonsOnly);
		}
	}
	return;
}

int SerialManagerBase::readStreamIntArray(int idx, int* arr, int len) {
  int i;
  for (i=0; i<len; i++) {
    arr[i] = *((int*)(stream_data+idx)); 
    idx=idx+4;
  }
  return idx;
}

int SerialManagerBase::readStreamFloatArray(int idx, float* arr, int len) {
  int i;
  for (i=0; i<len; i++) {
    arr[i] = *((float*)(stream_data+idx)); 
    idx=idx+4;
  }
  return idx;
}

//Send button state.  Transmit immediate, or queue up the messages for batch transfer.
void SerialManagerBase::setButtonState(String btnId, bool newState,bool sendNow) {
  
  //Create the new message
  String msg;
  if (newState) {
    msg.concat("STATE=BTN:" + btnId + ":1");
  } else {
    msg.concat("STATE=BTN:" + btnId + ":0");
  }

  //add to the TX buffer
  //if (TX_string.length() > 0) TX_string.concat('\n'); //seperate from previous messages
  TX_string.concat(msg); //add current message to the TX buffer


  //send or queue the message
  if (sendNow) {
    sendTxBuffer();
  } else {
    Serial.print("SerialManagerBase: queuing: "); Serial.println(msg);
  }
}

void SerialManagerBase::setButtonText(String btnId, String text) {
  String msg = "TEXT=BTN:" + btnId + ":" + text;
  Serial.println("SerialManagerBase: sending: " + msg);
  if (ble) ble->sendMessage(msg);
}

void SerialManagerBase::sendTxBuffer(void) {
  if (TX_string.length() > 0) {
    Serial.println("SerialManagerBase: sending: " + TX_string);
    if (ble) ble->sendMessage(TX_string);  //send the message
  }
  TX_string.remove(0,TX_string.length()); //clear out the string
}

