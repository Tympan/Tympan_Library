
#include "BLE/ble_nRF52.h"


char foo_digit2ascii(uint8_t digit) {
  return ( digit + ((digit) < 10 ? '0' : ('A'-10)) );
}

int printByteArray(uint8_t const bytearray[], int size)
{
  Serial.print("BLE_local: printByteArray: ");
  while(size--)
  {
    uint8_t byte = *bytearray++;
    Serial.write( foo_digit2ascii((byte & 0xF0) >> 4) );
    Serial.write( foo_digit2ascii(byte & 0x0F) );
    if ( size!=0 ) Serial.write('-');
  }
  Serial.println();

  return (size*3) - 1;
}

int printString(const String &str) {
  char c_str[str.length()];
  str.toCharArray(c_str, str.length());
  return printByteArray((uint8_t *)c_str,str.length());
}


int BLE_nRF52::begin_basic(void) {
	//Serial.println("BLE_nRF52: begin_basic: starting begin_basic()...");
	int ret_val = 0;
	
	//THIS IS THE KEY ACTIVITY:  Send begin() command to the BLE module
	String begin_reply;
	sendCommand("BEGIN ","1");delay(5);
  recvReply(&begin_reply); 
	if (doesStartWithOK(begin_reply)) {  
		//it's good!
		ret_val = 0; 
	} else { 
		//we didn't get an OK response.  it could be because the firmware is too old
		String version_str;
		int err_code = version(&version_str);
		bool suppress_warning_because_firmware_is_old = false;
		if (err_code == 0) {
			//we got the version info.
			//
			//strip off the leading text by finding the first space
			int ind = version_str.indexOf(" ");
			if (ind >= 0) {
				version_str = version_str.substring(ind+1);
				//find the first "v" to mark the version
				ind = version_str.indexOf("v");
				if (ind >= 0) {
					version_str = version_str.substring(ind+1);
					if (version_str.length() >= 3) {
						float version_float = version_str.substring(0,3).toFloat();
						if (version_float >= 0.4) {
							//it should have the BEGIN command, yet it failed, so do not suppress the warning
						} else {
							//it is old enough to not have the BEGIN command, so that's why it failed, so do suppress the warning
							suppress_warning_because_firmware_is_old = true;
							ret_val = 0;
						}
					}
				}
			}
		}
		if (!suppress_warning_because_firmware_is_old) {
			ret_val = -1; Serial.println("BLE_nRF52: begin: failed to begin.  Error msg = " + begin_reply);
		}
	}
	return ret_val;
}


int BLE_nRF52::begin(int doFactoryReset) {
	//Serial.println("BLE_nRF52: begin: starting begin()...");
	int ret_val = -1;
	
	String reply;  //we might need this later
	//clear the incoming Serial buffer	
	delay(2000);  //the UART serial link seems to drop the messages if we wait any less time than this
	while (serialFromBLE->available()) serialFromBLE->read();
	
	//if desired, send command to get the BLE version
	if (0) {
		int err_code = version(&reply);  
		if (err_code == 0) Serial.println("BLE_nRF52: begin: BLE firmware = " + reply);
	}
	
	//if desired send command to modify the advertised MAC addressed
	if (0) {
		setBleMac("AABBCCDDEEFF"); //set the MAC address that the BLE module advertises
	}
	
	//if desired, send commands to add another BLE service and to advertise that new service
	if (0) {
		int target_service_id = BLESVC_LEDBUTTON;
		enableServiceByID(target_service_id, true);    //enable the service 
		enableAdvertiseServiceByID(target_service_id); //tell the BLE module that this is the service we want it to advertise
	}

	// here is the core activity
	ret_val = begin_basic();
	
	
	//if desired, change the BLE name
	if (0) {
		int err_code = setBleName("MyBleName");
		if (err_code != 0) Serial.println("ble_NRF52: begin:  ble_nRF52.setBleName returned error code " + String(err_code));
	}
	
	//if desired, send the first data to service ID 5
	if (0) {
		int n_bytes = 1; uint8_t data[n_bytes] = {0x00};
		sendCommand("BLENOTIFY 5 0 1 ",data, n_bytes);   //service 5, characteristic 0, 1 byte
		sendCommand("BLEWRITE 5 0 1 ",data, n_bytes);   //service 5, characteristic 0, 1 byte
		delay(1);recvReply(&reply); if (doesStartWithOK(reply)) {  ret_val = 0; } else { ret_val = -1; Serial.println("BLE_nRF52: begin: failed to send data to service 5.  Reply = " + reply);}
	} 

	//if desired, send the first data to service ID 6
	if (0) {
		int n_bytes = 4; uint8_t data[n_bytes] = {0x00, 0x00, 0xc8, 0x42}; //floating point for 100.0, reverse byte order
		sendCommand("BLENOTIFY 6 0 4 ",data, n_bytes);   //service 6, characteristic 0, 4 bytes
		sendCommand("BLEWRITE 6 0 4 ",data, n_bytes);   //service 6, characteristic 0, 4 bytes
		delay(1);recvReply(&reply); if (doesStartWithOK(reply)) {  ret_val = 0; } else { ret_val = -1; Serial.println("BLE_nRF52: begin: failed to send data to service 6.  Reply = " + reply);}
	} 

	return ret_val;
}


void BLE_nRF52::setupBLE(int BT_firmware, bool printDebug, int doFactoryReset) {
	//Serial.println("ble_nRF52: begin: PIN_IS_CONNECTED = " + String(PIN_IS_CONNECTED)); 
	if (PIN_IS_CONNECTED >= 0) pinMode(PIN_IS_CONNECTED, INPUT); 
	begin(doFactoryReset);
}; 

size_t BLE_nRF52::send(const String &str) {

  //Serial.println("BLE_nRF52: send: " + str);
  //printString(str);

  int n_sent = 0;
  n_sent += serialToBLE->write("SEND ",4+1);  //out AT command set assumes that each transmission starts with "SEND "
  n_sent += serialToBLE->write(str.c_str(), str.length());
  n_sent += serialToBLE->write(EOC.c_str(), EOC.length());  // our AT command set on the nRF52 assumes that each command ends in a '\r'
  serialToBLE->flush();
//  Serial.println("BLE_nRF52: send: sent " + String(n_sent) + " bytes");
  delay(20); //This is to slow down the transmission to ensure we don't flood the bufers.  Can we get rid of this? 

  return str.length();
}

size_t BLE_nRF52::queue(const String &str) {
  //Serial.println("BLE_nRF52: queue: " + str);
  //printString(str);

  int n_sent = 0;
  n_sent += serialToBLE->write("QUEUE ",5+1);  //out AT command set assumes that each transmission starts with "SEND "
  n_sent += serialToBLE->write(str.c_str(), str.length());
  n_sent += serialToBLE->write(EOC.c_str(), EOC.length());  // our AT command set on the nRF52 assumes that each command ends in a '\r'
  serialToBLE->flush();
	delay(1);  //I still get errors during longer transmissions with no delay
//  Serial.println("BLE_nRF52: queue: sent " + String(n_sent) + " bytes");

  return str.length();
}

size_t BLE_nRF52::sendString(const String &s, bool print_debug) {
  int ret_val = send(s);
  if (ret_val == (int)(s.length()) ) {
    return ret_val;  //if send() returns non-zero, send the length of the transmission
  } else {
    if (print_debug) Serial.println("BLE: sendString: ERROR sending!  ret_val = " + String(ret_val) + ", string = " + s);
  }
  return 0;   //otherwise return zero (as a form of error?)
}


size_t BLE_nRF52::queueString(const String &s, bool print_debug) {
  int ret_val = queue(s);
  if (ret_val == (int)(s.length()) ) {
    return ret_val;  //if send() returns non-zero, send the length of the transmission
  } else {
    if (print_debug) Serial.println("BLE: queueString: ERROR sending!  ret_val = " + String(ret_val) + ", string = " + s);
  }
  return 0;   //otherwise return zero (as a form of error?)
}

size_t BLE_nRF52::transmitMessage(const String &orig_s, bool flag_useQueue) {
  String s = orig_s;
  const int payloadLen = 18; //was 19
  size_t sentBytes = 0;

  //Serial.println("BLE_nRF52: transmitMessage: commanded to send: " + orig_s);
	//if (flag_useQueue) {
	//  queue("");  queue("");  //send some blanks to clear
	//} else {
  //  send("");  send(""); //send some blanks to clear
	//

  //begin forming the fully-formatted string for sending to Tympan Remote App
  String header;
  header.concat('\xab'); header.concat('\xad'); header.concat('\xc0');header.concat('\xde'); // ABADCODE, message preamble
  header.concat('\xff');       // message type

  // message length
  if (s.length() >= (0x4000 - 1))  { //we might have to add a byte later, so call subtract one from the actual limit
      Serial.println("BLE_nRF52::transmitMessage: flag_useQueueMessage is too long!!! Aborting.");
      return 0;
  }
  int lenBytes = (s.length() << 1) | 0x8001; //the 0x8001 is avoid the first message having the 2nd-to-last byte being NULL
  header.concat((char)highByte(lenBytes));
  header.concat((char)lowByte(lenBytes));

  //check to ensure that there isn't a NULL or a CR in this header
  if ((header[6] == '\r') || (header[6] == '\0')) {
      //add a character to the end to avoid an unallowed hex code code in the header
      Serial.println("BLE_nRF52::transmitMessage ***WARNING*** message is being padded with a space to avoid its length being an unallowed value.");
      s.concat(' '); //append a space character

      //regenerate the size-related information for the header
      int lenBytes = (s.length() << 1) | 0x8001; //the 0x8001 is avoid the first message having the 2nd-to-last byte being NULL
      header[5] = ((char)highByte(lenBytes));
      header[6] = ((char)lowByte(lenBytes));
  }

  if (0) {
		//print debugging info
    Serial.println("BLE_nRF52::transmitMessage: ");
    Serial.println("    : Header length = " + String(header.length()) + " bytes");
    Serial.println("    : header (String) = " + header);
    Serial.print(  "    : header (HEX) = ");
    for (int i=0; i< int(header.length()); i++) Serial.print(header[i],HEX);
    Serial.println();
    Serial.println("    : Message: '" + s + "'");
  }

  //send the packet with the header information
  //Question: is "buf" actually used?  It doesn't look like it.  It looks like only "header" is used.
  char buf[21];  //was 16, which was too small for the 21 bytes that seem to be provided below
  sprintf(buf, "%02X %02X %02X %02X %02X %02X %02X", header.charAt(0), header.charAt(1), header.charAt(2), header.charAt(3), header.charAt(4), header.charAt(5), header.charAt(6)); 
	int a;
	if (flag_useQueue) { a = queueString(header); } else { a = sendString(header); } 
  if (a != 7) Serial.println("BLE_nRF52: transmitMessage: Error (a=" + String(a) + ") in sending message: " + String(header));  
 
  //break up String into packets
  int numPackets = ceil(s.length() / (float)payloadLen);
  for (int i = 0; i < numPackets; i++)  {
		String bu = String((char)(0xF0 | lowByte(i)));
		bu.concat(s.substring(i * payloadLen, (i * payloadLen) + payloadLen));
		if (flag_useQueue) { sentBytes += (queueString(bu) - 1); } else { sentBytes += (sendString(bu) - 1); } 
		//delay(4); //removed this line as delay() is applied in send() (within sendString) or by the BLE module itself (for queue() and queueString())
  }

  //Serial.print("BLE: transmitMessage: sentBytes = "); Serial.println((unsigned int)sentBytes);
  if (s.length() == sentBytes)
      return sentBytes;

  return 0;
}

size_t BLE_nRF52::sendCommand(const String &cmd, const String &data) {
  size_t len = 0;
	Serial.println("BLE_nRF52::sendCommand(String, String): " + cmd + data);
	
  //usualy something like cmd="SET NAME=" with data = "MY_NAME"
  //or something like cmd="GET NAME" with data =''
	if (0) {
		len += serialToBLE->write(cmd.c_str(), cmd.length() );
		for (unsigned int i=0; i<data.length(); ++i) len += serialToBLE->write(data[i]);  //be sure to send as invidual bytes
		serialToBLE->write(EOC.c_str(),EOC.length());  // our AT command set on the nRF52 assumes that each command ends in a '\r'
  } else {
		len += BLE_nRF52::sendCommand(cmd, data.c_str(), data.length()); //use this form to ensure that non-printable characters in data (especially 0x00) are transmitted correctly
	}
  return len;
}

size_t BLE_nRF52::sendCommand(const String &cmd, const char* data, size_t data_len) {
	if (0) {
		//print out the command, for debugging
		Serial.print("BLE_nRF52: sendCommand(string, char*,size_t): sending: " + cmd);
		for (size_t i=0; i<data_len;i++) Serial.write(data[i]);
		Serial.println();
	}
	return BLE_nRF52::sendCommand(cmd, (uint8_t*) data, data_len);
}

size_t BLE_nRF52::sendCommand(const String &cmd, const uint8_t* data, size_t data_len) {
	//Serial.print("BLE_nRF52: sendCommand(String,uint8,size_t): " + cmd); for (size_t i=0; i<data_len; i++) { Serial.write(data[i]); }; Serial.println();
  size_t len = 0;
	len += serialToBLE->write( cmd.c_str(), cmd.length() );
	len += serialToBLE->write(data,data_len);
  serialToBLE->write(EOC.c_str(),EOC.length());  // our AT command set on the nRF52 assumes that each command ends in a '\r'
  return len;	
}


int BLE_nRF52::recvReply(String *reply_ptr, unsigned long timeout_millis) {
  unsigned long max_millis = millis() + timeout_millis;
  bool waiting_for_EOC = true;
  bool waiting_for_O_or_F = true;
	reply_ptr->remove(0,reply_ptr->length());
  while ((millis() < max_millis) && (waiting_for_EOC)) {
    while (serialFromBLE->available()) {
      char c = serialFromBLE->read();
      if (waiting_for_O_or_F) {
        if ((c=='O') || (c=='F')) {
          waiting_for_O_or_F = false;
        }
        if (c != EOC[0]) reply_ptr->concat(c);
      } else {
        if (c == EOC[0]) { //this is a cheat!  someone, someday might want a two-character EOC (such as "\n\r"), at which point this will fail
          waiting_for_EOC = false;
        } else {
          reply_ptr->concat(c);
        }
      }
    }
  }
	bool flag_timedOut = (waiting_for_EOC == true);

  //clear out additional trailing whitespace from the stream
  while (serialFromBLE->available()) {
    int n_removed = 0;
    char c = serialFromBLE->peek();
    if ((c==(char)0x0D) || (c==(char)0x0A) || (c==' ') || (c=='\t')) {
      //remove it from the stream
      c = serialFromBLE->read();
      n_removed++;
    }
    //Serial.println("BLE_nRF52: removed additional " + String(n_removed) + " whitespace characters from serial stream.");
  }

  //remove leading or trailing white space from the reply itself
  reply_ptr->trim();
	
		
  //Serial.println("BLE_nRF52: recvReply: reply: " + *reply_ptr);
	if (flag_timedOut) return -1;
  return (int)reply_ptr->length();
}


bool BLE_nRF52::doesStartWithOK(const String &s) {
  return ((s.length() >= 2) && (s.substring(0,2)).equals("OK"));
}

// Grab an EOL-delimited string of output from the serial line
//          reply_ptr will contain the string on SUCCESS
//int BLE_nRF52::recv(String &reply) { return recv(&reply); }
int BLE_nRF52::recv(String *reply_ptr)
{
    if (!reply_ptr) // don't do this
        return -1;

    // start our timer
    unsigned long startTime = millis();

    // as long as we have time
    while ((startTime + _timeout) > millis())
    {
        // grab a character and append it to the current reply_ptr
        if (serialFromBLE->available() > 0)
        {
            reply_ptr->concat((char)serialFromBLE->read());
        }

        // we've reached the EOL sequence
        if (reply_ptr->endsWith(EOC))
        {
            // Lop off the EOL, since it is added by the BC127 and is not part of the characteristic payload
            //*reply_ptr = reply_ptr->substring(0,(reply_ptr->length())-EOC.length());
            reply_ptr->remove( (reply_ptr->length()) - EOC.length() );
            return 0;
        }
    }
    return -2;
}

size_t BLE_nRF52::recvBLE(String *s_ptr, bool printResponse) {
	String tmp = String("");

	// get our start time
	unsigned long startTime = millis();

	// as long as we have time
	while ((startTime + _timeout) > millis())  {
		if (serialFromBLE->available()) {
			while (serialFromBLE->available()) {
				s_ptr->concat((char)serialFromBLE->read());
				
				//do we end early because we've received on end-of-line character?
				if (s_ptr->length() >= EOC.length()+1) { //must have some payload in addition to the EOC 
					if (s_ptr->substring(s_ptr->length()-EOC.length(),s_ptr->length()) == EOC) {
						//remove EOC and return
						s_ptr->remove(s_ptr->length()-EOC.length());
						if (printResponse) Serial.println("BLE_nRF52: recvBLE: received = " + *s_ptr);
						return s_ptr->length();
					}
				}
			}
			if (printResponse) Serial.println("BLE_nRF52: recvBLE: received = " + *s_ptr);
			return s_ptr->length();
		}
	}
	if (printResponse) Serial.println("BLE_nRF52: recvBLE: timeout: nothing received.");
	return 0;
}
	

int BLE_nRF52::setBleName(const String &s) {
  sendCommand("SET NAME=", s);
  //if (simulated_Serial_to_nRF) bleUnitServiceSerial();  //for the nRF firmware, service any messages coming in the serial port from the Tympan
  
  String reply;
  int ret_val = recvReply(&reply);
  if (doesStartWithOK(reply)) {  
    return 0;
  } else {
		if (ret_val == -1) {
			Serial.println("BLE_nRF52: setBleName: recvReply() timed out.  Failed to set the BLE name.  Reply = " + reply);
		} else {
			Serial.println("BLE_nRF52: setBleName: Failed to set the BLE name.  Reply = " + reply);
		}
  }
  return -1;
}

int BLE_nRF52::getBleName(String *reply_ptr) {
  sendCommand(String("GET NAME"), String(""));
  //if (simulated_Serial_to_nRF) bleUnitServiceSerial();  //for the nRF firmware, service any messages coming in the serial port from the Tympan
  
  recvReply(reply_ptr); //look for "OK MY_NAME"
  //Serial.println("BLE_nRF52: getBleName: received raw reply_ptr = " + *reply_ptr);
  if (doesStartWithOK(*reply_ptr)) {
    reply_ptr->remove(0,2);  //remove the leading "OK"
    if ((reply_ptr->length() > 0) && (reply_ptr->charAt(0)==' ')) reply_ptr->remove(0,1);  //if present, remove the space that should have been after "OK"
    return 0;
  } else {
    Serial.println("BLE_nRF52: getBleName: failed to get the BLE name.  Reply = " + *reply_ptr);
    if (reply_ptr->length() > 0) reply_ptr->remove(0,reply_ptr->length());  //empty the string
  }
  return -1;
}

int BLE_nRF52::setBleMac(const String &s) {
  sendCommand("SET MAC=", s);
 
  String reply;
  recvReply(&reply);
  if (doesStartWithOK(reply)) {  
    return 0;
  } else {
    Serial.println("BLE_nRF52: setBleName: failed to set the BLE MAC.  Reply = " + reply);
  }
  return -1;
}

/*
int BLE_nRF52::getBleMac(String *reply_ptr) {
  sendCommand(String("GET MAC"), String(""));

  recvReply(reply_ptr); //look for "OK MY_NAME"
  //Serial.println("BLE_nRF52: getBleName: received raw reply_ptr = " + *reply_ptr);
  if (doesStartWithOK(*reply_ptr)) {
    reply_ptr->remove(0,2);  //remove the leading "OK"
    if ((reply_ptr->length() > 0) && (reply_ptr->charAt(0)==' ')) reply_ptr->remove(0,1);  //if present, remove the space that should have been after "OK"
    return 0;
  } else {
    Serial.println("BLE_nRF52: getBleName: failed to get the BLE MAC.  Reply = " + *reply_ptr);
    if (reply_ptr->length() > 0) reply_ptr->remove(0,reply_ptr->length());  //empty the string
  }
  return -1;
}
*/


/*
int BLE_nRF52::getLedMode(void) {
  sendCommand("GET LEDMODE",String(""));
  //if (simulated_Serial_to_nRF) bleUnitServiceSerial();  //for the nRF firmware, service any messages coming in the serial port from the Tympan
  
  String reply;
  recvReply(&reply); //look for "OK MY_NAME"
  if (doesStartWithOK(reply)) {
    reply.remove(0,2);  //remove the leading "OK"
    reply.trim(); //remove leading or trailing whitespace
    return int(reply.toInt());
  } 
  return -1; //error
}
*/

int BLE_nRF52::isConnected(int getMethod) {
  //Serial.println("BLE_nRF52: isConnected via method " + String(getMethod));
  if ((getMethod == GET_AUTO) || (getMethod == GET_VIA_GPIO)) {
    return isConnected_getViaGPIO();
  } else {
    return isConnected_getViaSoftware();
  }
  return -1; //error
}

int BLE_nRF52::isConnected_getViaSoftware(void) {
  sendCommand("GET CONNECTED",String(""));
  //if (simulated_Serial_to_nRF) bleUnitServiceSerial();  //for the nRF firmware, service any messages coming in the serial port from the Tympan
  
  String reply;
  recvReply(&reply); //look for "OK MY_NAME"
  //Serial.println("BLE_nRF52: isConnected_getViaSoftware: received raw reply = " + reply);
  if (doesStartWithOK(reply)) {
    reply.remove(0,2);  //remove the leading "OK"
    reply.trim(); //remove leading or trailing whitespace
    if (reply == "TRUE") {
      return 1;
    } else {
      return 0;
    }
  } 
  return -1; //error
}

int BLE_nRF52::isConnected_getViaGPIO(void) {
  if (PIN_IS_CONNECTED >= 0) { //is the pin defined?
    return digitalRead(PIN_IS_CONNECTED);
  } else {
    return -1; //error
  }
}

int BLE_nRF52::isAdvertising(void) {
  sendCommand("GET ADVERTISING",String(""));
  //if (simulated_Serial_to_nRF) bleUnitServiceSerial();  //for the nRF firmware, service any messages coming in the serial port from the Tympan
  
  String reply;
  recvReply(&reply); //look for "OK MY_NAME"
  //Serial.println("BLE_nRF52: isAdvertising: received raw reply = " + reply);
  if (doesStartWithOK(reply)) {
    reply.remove(0,2);  //remove the leading "OK"
    reply.trim(); //remove leading or trailing whitespace
    if (reply == "TRUE") {
      return 1;
    } else {
      return 0;
    }
  } 
  return -1; //error
}

int BLE_nRF52::enableServiceByID(int service_id, bool enable) { 
	if ((service_id < 1) || (service_id > 9)) return -1;  //return with error if given id is out of this narrow range 
	
	//build up the command and send it
	char service_id_char = (char)((uint8_t)service_id + (uint8_t)'0');
	String true_false = "TRUE";
	if (!enable) true_false = "FALSE";
	sendCommand("SET ENABLE_SERVICE_ID" + String(service_id_char) + "=",true_false); delay(1);

	//receive reply
  String reply; recvReply(&reply);
	if (!doesStartWithOK(reply)) Serial.println("BLE_nRF52: enableServiceByID: failed to set service " + String(service_id_char) + " to " + true_false + ". Reply = " + reply);

	//return
  if (doesStartWithOK(reply)) return 0; //good result
  return -1; //error  //bad result
}

int BLE_nRF52::enableAdvertiseServiceByID(int service_id) {  
	if ((service_id < 1) || (service_id > 9)) return -1;  //return with error if given id is out of this narrow range 
	
	//build up the command and send it
	char service_id_char = (char)((uint8_t)service_id + (uint8_t)'0');
	sendCommand("SET ADVERT_SERVICE_ID=",service_id_char);

	//receive reply
  String reply; recvReply(&reply);
	if (!doesStartWithOK(reply)) Serial.println("BLE_nRF52: enableAdvertiseServiceByID: failed to set advertising for service " + String(service_id_char) + ". Reply = " + reply);

	//return
  if (doesStartWithOK(reply)) return 0; //good result
  return -1; //error  //bad result
}

int BLE_nRF52::enableAdvertising(bool enable) {
  if (enable) {
    sendCommand("SET ADVERTISING=","ON");
  } else {
    sendCommand("SET ADVERTISING=","OFF");
  }
  String reply;
  recvReply(&reply); //look for "OK MY_NAME"
  //Serial.println("BLE_nRF52: enableAdvertising: received raw reply = " + reply);
  if (doesStartWithOK(reply)) {
    return 0;
  }
  return -1; //error
}

/*
int BLE_nRF52::setLedMode(int value) {
  sendCommand("SET LEDMODE=",String(value));

  String reply;
  recvReply(&reply); //look for "OK""
  //Serial.println("BLE_nRF52: setLedMode: received raw reply = " + reply);
  if (doesStartWithOK(reply)) {
    return 0;
  }
  return -1; //error
}
*/

int BLE_nRF52::version(String *replyToReturn_ptr) {
	Serial.println("BLE_nRF52::version: sending GET VERSION...");
  sendCommand("GET VERSION",String(""));
  //if (simulated_Serial_to_nRF) bleUnitServiceSerial();  //for the nRF firmware, service any messages coming in the serial port from the Tympan
  
  String reply;
  int ret_val = recvReply(&reply); //look for "OK MY_NAME"
  //Serial.println("BLE_nRF52: version: received raw reply = " + reply);
  if (doesStartWithOK(reply)) {
    reply.remove(0,2);  //remove the leading "OK"
    reply.trim(); //remove leading or trailing whitespace
    //if (printResult) {
    //  Serial.println("BLE_nR52: version: nRF52 module replied as '" + reply + "'");
    //}
    replyToReturn_ptr->remove(0,replyToReturn_ptr->length()); //empty the return string
    replyToReturn_ptr->concat(reply);
    return 0;
  }
	
	//if we're here, it hasn't worked
	if (ret_val == -1) {
		Serial.println("BLE_nRF52: version: recvReply() timed out.  Failed to get the BLE firmware version.  Reply = " + reply);
	} else {
		Serial.println("BLE_nRF52: version: Failed to get the BLE firmware version.  Reply = " + reply);
	}
	if (replyToReturn_ptr->length() > 0) replyToReturn_ptr->remove(0,replyToReturn_ptr->length());  //empty the return string
  return -1;
}

int BLE_nRF52::clearTxQueue(String *replyToReturn_ptr) {
	sendCommand("CLEAR_TX_QUEUE",String(""));
	
	String reply;
  int ret_val = recvReply(&reply); //look for reply of the format "OK 0 15000"
  //Serial.println("BLE_nRF52: clearTxQueue: received raw reply = " + reply);
  if (doesStartWithOK(reply)) {
    reply.remove(0,2);  //remove the leading "OK"
    reply.trim(); //remove leading or trailing whitespace
    replyToReturn_ptr->remove(0,replyToReturn_ptr->length()); //empty the return string
    replyToReturn_ptr->concat(reply);
    return 0;
  }
	
	//if we're here, it hasn't worked
	if (ret_val == -1) {
		Serial.println("BLE_nRF52: clearTxQueue: recvReply() timed out.  Failed to clear the module's TX queue.  Reply = " + reply);
	} else {
		Serial.println("BLE_nRF52: clearTxQueue: Failed to clear the module's TX queue.  Reply = " + reply);
	}
	if (replyToReturn_ptr->length() > 0) replyToReturn_ptr->remove(0,replyToReturn_ptr->length());  //empty the return string
	return -1;
}


int BLE_nRF52::sendSetForIntegerValue(const String &parameter_str, const int value) {
	//Serial.print("BLE_nRF52::sendSetForIntegerValue: starting using " + parameter_str + " " + String(value)); Serial.print(", which was given as value "); Serial.println(value);
	sendCommand("SET " + parameter_str + "=", String(value)); 	//send the SET command
  String reply;  
	int ret_val = recvReply(&reply); 	//get the reply to the command
  if (doesStartWithOK(reply)) return 0;  //success!

	//fail
	if (ret_val == -1)  {
		Serial.println("BLE_nRF52: sendSetForIntegerValue: recvReply() timed out.  Failed to set " + parameter_str + ".  Reply = " + reply);
	} else {
		Serial.println("BLE_nRF52: sendSetForIntegerValue: Failed to set " + parameter_str + ".  Reply = " + reply);
  }
	return -1;	 //fail
}


int BLE_nRF52::sendGetForIntegerValue(const String &parameter_str) {
	sendCommand("GET " + parameter_str, String("")); 	//send the GET command
  String reply; 
	int ret_val = recvReply(&reply);  //get the reply to the command
	if (doesStartWithOK(reply)) {     //does it start with OK?
    reply.remove(0,2);              //remove the leading "OK"
    reply.trim();                   //remove leading or trailing whitespace
		return int(reply.toInt());      //extract the value and return
  } 
	
	//fail
	if (ret_val == -1)  {
		Serial.println("BLE_nRF52: sendSetForIntegerValue: recvReply() timed out.  Failed to get " + parameter_str + ".  Reply = " + reply);
	} else {
		Serial.println("BLE_nRF52: sendSetForIntegerValue: Failed to get " + parameter_str + ".  Reply = " + reply);
  }
  return -1;
}	

int BLE_nRF52::setTxQueueDelay_msec(const int delay_msec) {
	if ((delay_msec < 0) || (delay_msec > 99)) {
		Serial.println("BLE_nRF52::setQueueDelay_msec: given delay_msec must be between 0 and 99 msec.  Returning.");
		return -1; //fail
	}
	return sendSetForIntegerValue("DELAY_MSEC", delay_msec);
}

int BLE_nRF52::getModuleBufferOverflows(int max_allowed_out_vals, int *out_vals, int *num_out_vals) {
	sendCommand("GET BUFF_OVERFLOWS", String("")); 	//send the GET command
  String reply; 
	int ret_val = recvReply(&reply);  //get the reply to the command
	//Serial.println("BLE_nRF52::getModuleBufferOverflows: reply = " + reply);
	
	//if reply is OK, retried the unknown number of integer values that might be present
	if (doesStartWithOK(reply)) {     //does it start with OK?
    reply.remove(0,2);              //remove the leading "OK"
    reply.trim();                   //remove leading or trailing whitespace
		int n_vals = 0;
		while ((reply.length() > 0) && (n_vals < max_allowed_out_vals)) {
			reply.trim(); //remove leading or trailing whitespace
			if (reply.length() > 0) {
				int ind_white = reply.indexOf(" ");
				if (ind_white < 0) {
					//no white space, so try to interpret the remaining string
					if ((reply[0] >= '0') && (reply[0] <= '9')) {
						out_vals[n_vals] = reply.toInt();
						n_vals++;
					}
					reply.remove(0); //clear out the string
				} else {
					//extract the substring
					String foo_str = (reply.substring(0, ind_white)).trim();
					if (foo_str.length() > 0) {
						if ((foo_str[0] >= '0') && (foo_str[0] <= '9')) {
							out_vals[n_vals] = foo_str.toInt();
							n_vals++;
						}
					}
					reply.remove(0,ind_white); //remove the token that we just analyzed
				}
			}	
		}
		*num_out_vals = n_vals;
		return 0;
  } 
	
	//fail
	if (ret_val == -1)  {
		Serial.println("BLE_nRF52: getModuleBufferOverflows: recvReply() timed out.  Failed to get BUFF_OVERFLOWS. Reply = " + reply);
	} else {
		Serial.println("BLE_nRF52: getModuleBufferOverflows: Failed to get BUFF_OVERFLOWS.  Reply = " + reply);
  }
  return -1;
}


uint32_t floatToBytesInUint32(float32_t value) {  uint32_t result;  memcpy(&result, &value, sizeof(value)); return result; }
uint32_t floatToBytesInInt32(float32_t value) {  uint32_t result;  memcpy(&result, &value, sizeof(value)); return result; }
void convertToByteArray(uint32_t val_uint32, uint8_t *array_4bytes) {
	const uint8_t n_bytes = 4;
	for (uint8_t Ibyte = 0; Ibyte < n_bytes; ++Ibyte) {
		array_4bytes[Ibyte]=(uint8_t)(0x000000FF & (val_uint32 >> (Ibyte*8)));  //lower bytes first
	}
}
void convertToByteArray(int32_t val, uint8_t *array_4bytes) {
	uint32_t val_uint32;  
	memcpy(&val_uint32, &val, sizeof(val));	
	convertToByteArray(val_uint32, array_4bytes);
}
void convertToByteArray(float val, uint8_t *array_4bytes) {
	uint32_t val_uint32;  
	memcpy(&val_uint32, &val, sizeof(val));	
	convertToByteArray(val_uint32, array_4bytes);
}

int BLE_nRF52::notifyBle(int service_id, int char_id, float32_t val) {  //notify, float32
	const size_t n_bytes = 4;	uint8_t byte_array[n_bytes];
  convertToByteArray(val,byte_array);
	return notifyBle(service_id, char_id, byte_array, n_bytes);
}
int BLE_nRF52::notifyBle(int service_id, int char_id, int32_t val) {  //notify, int32
	const size_t n_bytes = 4;	uint8_t byte_array[n_bytes];
  convertToByteArray(val,byte_array);
	return notifyBle(service_id, char_id, byte_array, n_bytes);
}
int BLE_nRF52::notifyBle(int service_id, int char_id, uint32_t val) {  //notify, uint32
	const size_t n_bytes = 4;	uint8_t byte_array[n_bytes];
  convertToByteArray(val,byte_array);
	return notifyBle(service_id, char_id, byte_array, n_bytes);
}
//int BLE_nRF52::notifyBle(int service_id, int char_id, uint8_t val)   { 
//	//return notifyBle(service_id, char_id, &val, 1);	
//	const size_t n_bytes = 1;	uint8_t byte_array[n_bytes];
//	byte_array[0] = val;
//	return notifyBle(service_id, char_id, byte_array, n_bytes);
//}

int BLE_nRF52::writeBle(int service_id, int char_id, float32_t val) {  //write, float32
	const size_t n_bytes = 4;	uint8_t byte_array[n_bytes];
  convertToByteArray(val,byte_array);
	return writeBle(service_id, char_id, byte_array, n_bytes);
}
int BLE_nRF52::writeBle(int service_id, int char_id, int32_t val) {  //write, int32
	const size_t n_bytes = 4;	uint8_t byte_array[n_bytes];
  convertToByteArray(val,byte_array);
	return writeBle(service_id, char_id, byte_array, n_bytes);
}
int BLE_nRF52::writeBle(int service_id, int char_id, uint32_t val) {  //write, uint32
	const size_t n_bytes = 4;	uint8_t byte_array[n_bytes];
  convertToByteArray(val,byte_array);
	return writeBle(service_id, char_id, byte_array, n_bytes);
}
//int BLE_nRF52::writeBle(int service_id, int char_id, uint8_t val)   { 
//	//return notifyBle(service_id, char_id, &val, 1);	
//	const size_t n_bytes = 1;	uint8_t byte_array[n_bytes];
//	byte_array[0] = val;
//	return writeBle(service_id, char_id, byte_array, n_bytes);
//}
int BLE_nRF52::notifyBle(int service_id, int char_id, const uint8_t byte_array[], size_t n_bytes) {
	int ret_val = 0;
  String params_str = String(" ") + String(service_id) + " " + String(char_id) + " " + String(n_bytes) + " "; //start and end with a a space

  String command_str = "BLENOTIFY";
  //Serial.print("ble_nRF52: notifyBle: Full Command: " + command_str + params_str + ": "); for (int i=0; i<n_bytes;i++) { Serial.print("0x"); Serial.print(data_uint8[i],HEX); Serial.print(" ");}; Serial.println();
  sendCommand(command_str + params_str, byte_array, n_bytes);   
  delay(4); String reply; recvReply(&reply); 
	if (!doesStartWithOK(reply)) {
		ret_val = -1;
		Serial.println("BLE_nRF52: noitfyBle: failed send BLE data via " + command_str + ". Reply = " + reply);
	}
	return ret_val;
}
int BLE_nRF52::writeBle(int service_id, int char_id, const uint8_t byte_array[], size_t n_bytes) {
	int ret_val = 0;
  String params_str = String(" ") + String(service_id) + " " + String(char_id) + " " + String(n_bytes) + " "; //start and end with a a space

  String command_str = "BLEWRITE";
  //Serial.print("ble_nRF52: writeBle: Full Command: " + command_str + params_str + ": "); for (int i=0; i<n_bytes;i++) { Serial.print("0x"); Serial.print(data_uint8[i],HEX); Serial.print(" ");}; Serial.println();
  sendCommand(command_str + params_str, byte_array, n_bytes);   
  delay(4); String reply; recvReply(&reply); 
	if (!doesStartWithOK(reply)) {
		ret_val = -1;
		Serial.println("BLE_nRF52: writeBle: failed send BLE data via " + command_str + ". Reply = " + reply);
	}
	return ret_val;
}



int BLE_nR52_CustomService::setServiceUUID(const String &text) {
	int ret_val = 0;
	int char_id = 0; //it's used in the transmission, but it's ignored for this command
  String pre_str = String("SVCSETUP") + String(" ") + String(service_id) + " " + String(char_id) + " "; //start and end with a a space
	String command_str = "SERVICEUUID";
  if (this_nRF52) {
		this_nRF52->sendCommand(pre_str + command_str + "=", text);
		delay(4); String reply; this_nRF52->recvReply(&reply); 
		if (!this_nRF52->doesStartWithOK(reply)) {
			ret_val = -1;  Serial.println("BLE_nRF52: setServiceUUID: failed: " + command_str + ". Reply = " + reply);
		}
	} else {
		ret_val = -2;  //no valid ble pointer
	}
	return ret_val;
};

int BLE_nR52_CustomService::setServiceName(const String &text) {
	int ret_val = 0;
	int char_id = 0; //it's used in the transmission, but it's ignored for this command
  String pre_str = String("SVCSETUP") + String(" ") + String(service_id) + " " + String(char_id) + " "; //start and end with a a space
	String command_str = "SERVICENAME";
  if (this_nRF52) {
		this_nRF52->sendCommand(pre_str + command_str + "=", text);
		delay(4); String reply; this_nRF52->recvReply(&reply); 
		if (!this_nRF52->doesStartWithOK(reply)) {
			ret_val = -1;  Serial.println("BLE_nRF52: setServiceName: failed: " + command_str + ". Reply = " + reply);
		}
	} else {
		ret_val = -2;  //no valid ble pointer
	}
	return ret_val;
};

int BLE_nR52_CustomService::addCharacteristic(const String &text) {
	int ret_val = 0;
	int char_id = 0; //it's used in the transmission, but it's ignored for this command
  String pre_str = String("SVCSETUP") + String(" ") + String(service_id) + " " + String(char_id) + " "; //start and end with a a space
	String command_str = "ADDCHAR";
  if (this_nRF52) {
		this_nRF52->sendCommand(pre_str + command_str + "=", text);
		delay(4); String reply; this_nRF52->recvReply(&reply); 
		if (!this_nRF52->doesStartWithOK(reply)) {
			ret_val = -1;  Serial.println("BLE_nRF52: addCharacteristic: failed: " + command_str + ". Reply = " + reply);
		}
	} else {
		ret_val = -2;  //no valid ble pointer
	}
	return ret_val;
}; 
int BLE_nR52_CustomService::setCharName(const int char_id, const String &text) {
	int ret_val = 0;
  String pre_str = String("SVCSETUP") + String(" ") + String(service_id) + " " + String(char_id) + " "; //start and end with a a space
	String command_str = "CHARNAME";
  if (this_nRF52) {
		this_nRF52->sendCommand(pre_str + command_str + "=", text);
		delay(4); String reply; this_nRF52->recvReply(&reply); 
		if (!this_nRF52->doesStartWithOK(reply)) {
			ret_val = -1;  Serial.println("BLE_nRF52: setCharName: failed: " + command_str + ". Reply = " + reply);
		}
	} else {
		ret_val = -2;  //no valid ble pointer
	}
	return ret_val;
};
int BLE_nR52_CustomService::setCharProperties(const int char_id, const String &text) {
	int ret_val = 0;
  String pre_str = String("SVCSETUP") + String(" ") + String(service_id) + " " + String(char_id) + " "; //start and end with a a space
	String command_str = "CHARPROPS";
  if (this_nRF52) {
		this_nRF52->sendCommand(pre_str + command_str + "=", text);
		delay(4); String reply; this_nRF52->recvReply(&reply); 
		if (!this_nRF52->doesStartWithOK(reply)) {
			ret_val = -1;  Serial.println("BLE_nRF52: setCharProperties: failed: " + command_str + ". Reply = " + reply);
		}
	} else {
		ret_val = -2;  //no valid ble pointer
	}
	return ret_val;
};
int BLE_nR52_CustomService::setCharNBytes(const int char_id, const int n_bytes) {
	int ret_val = 0;
  String pre_str = String("SVCSETUP") + String(" ") + String(service_id) + " " + String(char_id) + " "; //start and end with a a space
	String command_str = "CHARNBYTES";
  if (this_nRF52) {
		this_nRF52->sendCommand(pre_str + command_str + "=", String(n_bytes));
		delay(4); String reply; this_nRF52->recvReply(&reply); 
		if (!this_nRF52->doesStartWithOK(reply)) {
			ret_val = -1;  Serial.println("BLE_nRF52: setCharNBytes: failed: " + command_str + ". Reply = " + reply);
		}
	} else {
		ret_val = -2;  //no valid ble pointer
	}
	return ret_val;
};


// /////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// UI Methods
//
// 
// /////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BLE_nRF52_UI::printHelp(void) {
	String prefix = getPrefix();  //getPrefix() is in SerialManager_UI.h, unless it is over-ridden in this class somewhere
	Serial.println(F(" BLE_nRF52_UI: Prefix = ") + prefix);
	Serial.println(F("   s:   Print Bluetooth status"));
	Serial.println(F("   v:   Print Bluetooth Firmware version info"));
	Serial.println(F("   a/A: Activate/Deactivate BLE advertising"));
}


bool BLE_nRF52_UI::processCharacterTriple(char mode_char, char chan_char, char data_char) {
	//check the mode_char to see if it corresponds with this instance of this class.  If not, return with no action.
	if (mode_char != ID_char) return false;  //ID_char is from SerialManager_UI.h
	
	return processSingleCharacter(data_char);
}

//respond to serial commands
bool BLE_nRF52_UI::processSingleCharacter(char c) {
  
	bool ret_val = true;
	switch (c) {
		case 'a':
			Serial.println("BLE_nRF52: activating BLE advertising...");
			enableAdvertising(true);
			break;
		case 'A':
			Serial.println("BLE_nRF52: de-activating BLE advertising...");
			enableAdvertising(false);
			break;
		case 's':
			Serial.println("BLE_nRF52: " + String(c) + " not yet implemented");
			//Serial.println("BLE: printing Bluetooth status...");
			//status(true);
			break;     
		case 'v':
			Serial.println("BLE_nRF52: getting Bluetooth firmware version info...");
			{	
				String reply;
				BLE_nRF52::version(&reply);
				Serial.println("BLE_nRF52: Version = " + reply);
			}
			break;
		default:
			ret_val = false;
		
	}
	return ret_val;
}
