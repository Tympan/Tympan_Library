
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
	const char context_txt[] = "SerialManagerBase: RespondToByte: ";
	
  switch (serial_read_state) {
    case SINGLE_CHAR:
      if (c == DATASTREAM_START_CHAR) {
        //Serial.print(context_txt);Serial.print("Start data stream.");
        // Start a data stream:
        serial_read_state = STREAM_LENGTH;
        stream_chars_received = 0;
      } else if (c == QUADCHAR_START_CHAR) {
        serial_read_state = QUAD_CHAR_1;
      } else {
        if ((c != '\r') && (c != '\n')) {
          //Serial.print(context_txt);Serial.print("Processing character "); Serial.println(c);
          processCharacter(c);
        }
      }
      break;
    case STREAM_LENGTH:
      if (c == DATASTREAM_SEPARATOR) {
        // Get the datastream length:
        if (stream_chars_received==4) {
          memcpy(&stream_length, &stream_data[0], 4);
          serial_read_state = STREAM_DATA;
          stream_chars_received = 0;
          //Serial.print(context_txt);Serial.print("Stream length = ");  Serial.println(stream_length);
        } 
        // Error, length must be 4-byte int
        else {
          Serial.print(context_txt);Serial.println(F("Error: Datastream message length must be 4 byte Int"));
          //Reset for next message
          serial_read_state = SINGLE_CHAR;
          stream_chars_received = 0;
        }

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
          //Serial.print(context_txt);Serial.println(F("Time to process stream!"));
          processStream();
        } else {
          Serial.print(context_txt);Serial.print(F("ERROR: Expected string terminator "));
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
      if (c == DATASTREAM_START_CHAR) { Serial.print(context_txt);Serial.println("Start data stream."); serial_read_state = STREAM_LENGTH; stream_chars_received = 0; return; }//abort into stream mode
      mode_char = c; chan_char = (char)NULL; data_char = (char)NULL;
      serial_read_state = QUAD_CHAR_2;
      break;
    case QUAD_CHAR_2:
      if ((c == '\n') || (c == '\r')) { serial_read_state = SINGLE_CHAR; return; } // abort
      if (c == DATASTREAM_START_CHAR) { Serial.print(context_txt);Serial.println("Start data stream."); serial_read_state = STREAM_LENGTH; stream_chars_received = 0; return;} //abort into stream mode
      chan_char = c;
      serial_read_state = QUAD_CHAR_3;
      break;
    case QUAD_CHAR_3:
      if ((c == '\n') || (c == '\r')) { serial_read_state = SINGLE_CHAR; return; } // abort
      if (c == DATASTREAM_START_CHAR) { Serial.print(context_txt);Serial.println("Start data stream."); serial_read_state = STREAM_LENGTH; stream_chars_received = 0; return; }//abort into stream mode
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
  int tmpInt = 0;
  float tmpFloat = 0;
	const char context_txt[] = "SerialManagerBase: processStream:";
  
  //Look for message type by finding the next separating character 
  while ( ((stream_data[idx] != DATASTREAM_SEPARATOR) && (stream_data[idx] != ' ')) && (idx < stream_length) ) {
    streamType.append(stream_data[idx]);
    idx++;
  }
  idx++; // move past the datastream separator

  //Serial.print(context_txt); Serial.println("streamType = " + streamType);
	//Serial.print("Received stream: "); Serial.println(stream_data);
	if (streamType == "BLEDATA") {
		interpretBleData(idx);
//  } else if (streamType == "gha") {    
//    Serial.print(context_txt); Serial.println("Stream is of type 'gha'.");
//    //interpretStreamGHA(idx);
//  } else if (streamType == "dsl") {
//    Serial.print(context_txt); Serial.println("Stream is of type 'dsl'.");
//    //interpretStreamDSL(idx);
//  } else if (streamType == "afc") {
//    Serial.print(context_txt); Serial.println("Stream is of type 'afc'.");
//    //interpretStreamAFC(idx);
  } else if (streamType == "test") {    
    Serial.print(context_txt); Serial.println("Stream is of type 'test'.");

    //Print first 4-bytes as integer
    if ( (stream_length-idx) >= 4) {
      memcpy(&tmpInt, stream_data+idx, 4);
      idx = idx+4;
      Serial.print("int is "); Serial.println(tmpInt);
    }

    //Print the next 4-bytes as float
    if (stream_length-idx >= 4) {    
      memcpy(&tmpFloat, stream_data+idx, 4);
      Serial.print("float is "); Serial.println(tmpFloat);
    }
  
  // Else the message type was not identifued
  } else {
    // If a custom callback was set, call it
    if (_datastreamCallback_p!=NULL)
    {
      _datastreamCallback_p(stream_data+idx, &streamType, stream_length-idx);
    }

    // Else no message type found, so ignore it
    else
    {
      Serial.print(context_txt); Serial.print("Unknown stream type: ");
      Serial.println(streamType);
    }
  }
}

void SerialManagerBase::printStreamData(int idx) { for (int i=idx; i < stream_length; ++i) Serial.print(stream_data[i]); }
void printBleDataMessage(const BleDataMessage &msg) {
	Serial.print("service_id = " + String(msg.service_id) +
	             ", char_id = " + String(msg.characteristic_id) +
							 ", payload = ");
	for (auto val : msg.data) Serial.print(val); 
}
	
	
int charArrayToInt(char *char_array, int start_ind, int end_ind) {
	if (start_ind >= end_ind) return 0;
	int out_val = 0;
	bool is_negative = false;
	if (char_array[0] == '-') is_negative = true;
	for (int i = start_ind; i < end_ind; ++i) {
		char c = char_array[i];
		if ((c >= '0') && (c <= '9')) {
			out_val *= 10;
			out_val += (int)(c - '0');
		}
	}
	if (is_negative) out_val *= -1;
	return out_val;
}

int SerialManagerBase::interpretBleData(int idx) {
	int start_idx;
	const char context_txt[] = "SerialManagerBase: interpretBleData: ";
	BleDataMessage bleDataMessage;
	
	if (0) {
		//print out the stream data, for debugging
		Serial.print(context_txt);Serial.print("ble data = "); printStreamData(idx); Serial.println();
	}
	
	//get the BLE service
	start_idx = idx;
	while ( ((stream_data[idx] != DATASTREAM_SEPARATOR) && (stream_data[idx] != ' ')) && (idx < stream_length) ) idx++;
	if (idx >= stream_length) {
		Serial.print(context_txt);Serial.print("FAILED to interpret service_id of BLE message: "); printStreamData(0);Serial.println();
		return -1;
	}
	bleDataMessage.service_id = charArrayToInt(stream_data,start_idx,idx);
  idx++; // move past the datastream separator
		
	//get the BLE characteristic
	start_idx = idx;
	while ( ((stream_data[idx] != DATASTREAM_SEPARATOR) && (stream_data[idx] != ' ')) && (idx < stream_length) ) idx++;
	if (idx >= stream_length) {
		Serial.print(context_txt);Serial.print("FAILED to interpret characteristic id of BLE message: "); printStreamData(0);Serial.println();
		return -1;
	}
	bleDataMessage.characteristic_id = charArrayToInt(stream_data,start_idx,idx);
  idx++; // move past the datastream separator
	
	//get the number of bytes in the payload
	int n_bytes = stream_length - idx;
	
	//check that the size is OK
	start_idx = idx;
	if (n_bytes < 1) {
		Serial.print(context_txt);Serial.print("FAILED: BLE n_bytes (" + String(n_bytes) + ") must be greater than zero.");
		return -1;
	}
	if (n_bytes > MAX_BLE_PAYLOAD) {
		Serial.print(context_txt);Serial.print("FAILED: BLE n_bytes (" + String(n_bytes) + ") is more than allowed (" + String(MAX_BLE_PAYLOAD) + ")");
		return -1;	
	}
	
	//get the payload and copy into the vector
	bleDataMessage.data.reserve(n_bytes);
	for (int i=idx; i < stream_length; ++i) bleDataMessage.data.push_back(stream_data[i]);

	//print out the 
	if (0) {
		Serial.print(context_txt);
		printBleDataMessage(bleDataMessage);
		Serial.println();
	}
	
	//push it into the BLE message queue
	int err_code = bleDataMessageQueue.push(bleDataMessage);
	if (err_code != 0) { 
		Serial.print(context_txt); Serial.println("ERROR: Received BLE Message but the queue is full."); 
		return -1;
	}
	return 0; //zero is no error
}

BleDataMessage SerialManagerBase::getBleDataMessage(void) { 
	if (isBleDataMessageAvailable() == 0) {
		//return an empty message
		BleDataMessage msg;
		return msg;
	}
	BleDataMessage msg = bleDataMessageQueue.front(); //get the oldest message in the queue
	bleDataMessageQueue.pop();  //remove from front of the queue			
	return msg;
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


void SerialManagerBase::setDataStreamCallback(callback_t callBackFunc_p)
{
  Serial.print("Datastream callback set");
  _datastreamCallback_p = callBackFunc_p;
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