
#include <ble/ble_nRF52.h>


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

size_t BLE_nRF52::send(const String &str) {

  //Serial.println("BLE_local: send(string): ...);
  //printString(str);

  int n_sent = 0;
  n_sent += serialToBLE->write("SEND ",5);  //out AT command set assumes that each transmission starts with "SEND "
  n_sent += serialToBLE->write(str.c_str(), str.length());
  n_sent += serialToBLE->write(EOC.c_str(), EOC.length());  // our AT command set on the nRF52 assumes that each command ends in a '\r'
  serialToBLE->flush();
//  Serial.println("BLE_nRF52: send: sent " + String(n_sent) + " bytes");
  delay(20); //This is to slow down the transmission to ensure we don't flood the bufers.  Can we get rid of this? 

  //BLE_TX_ptr->update(0); //added WEA DEC 30, 2023
  //if (! BLE_TX_ptr->waitForOK() ) {
  //  Serial.println(F("BLE_local: send: Failed to send?"));
  //}
  //ble.println(str, HEX);
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

size_t BLE_nRF52::sendMessage(const String &orig_s) {
  String s = orig_s;
  const int payloadLen = 18; //was 19
  size_t sentBytes = 0;

  //Serial.println("BLE_nRF52: sendMessage: commanded to send: " + orig_s);
  //send("");  send(""); //send some blanks to clear

  //begin forming the fully-formatted string for sending to Tympan Remote App
  String header;
  header.concat('\xab'); header.concat('\xad'); header.concat('\xc0');header.concat('\xde'); // ABADCODE, message preamble
  header.concat('\xff');       // message type

  // message length
  if (s.length() >= (0x4000 - 1))  { //we might have to add a byte later, so call subtract one from the actual limit
      Serial.println("BLE: Message is too long!!! Aborting.");
      return 0;
  }
  int lenBytes = (s.length() << 1) | 0x8001; //the 0x8001 is avoid the first message having the 2nd-to-last byte being NULL
  header.concat((char)highByte(lenBytes));
  header.concat((char)lowByte(lenBytes));

  //check to ensure that there isn't a NULL or a CR in this header
  if ((header[6] == '\r') || (header[6] == '\0')) {
      //add a character to the end to avoid an unallowed hex code code in the header
      Serial.println("BLE: sendMessage: ***WARNING*** message is being padded with a space to avoid its length being an unallowed value.");
      s.concat(' '); //append a space character

      //regenerate the size-related information for the header
      int lenBytes = (s.length() << 1) | 0x8001; //the 0x8001 is avoid the first message having the 2nd-to-last byte being NULL
      header[5] = ((char)highByte(lenBytes));
      header[6] = ((char)lowByte(lenBytes));
  }

  if (0) {
    Serial.println("BLE_local: sendMessage: ");
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
  int a = sendString(header); //for V5.5, this will return an error if there is no active BT connection
  //Serial.println("    : return value a = " + String(a));
  if (a != 7)  {
    //With v5, it returns an error if there is no BT connection.
    //Checking if there is a BT connection really slows down this transaction.
    //So, let's just suppress this error message for V5 and only allow for V7.
    //Then, for V7, it automatically tracks the connection status via having a
    //valid BLE_id_num.  So, only print an error message for V7 if there is indeed
    //a valid BLE_id_num.
    
    //if ((BC127_firmware_ver > 6) && (BLE_id_num > 0)) {
      Serial.println("BLE_nRF52: sendMessage: Error (a=" + String(a) + ") in sending message: " + String(header));  
    //} 
    //if we really do get an error, should we really try to transmit all the packets below?  Seems like we shouldn't.
  }
  

  //break up String into packets
  int numPackets = ceil(s.length() / (float)payloadLen);
  for (int i = 0; i < numPackets; i++)  {
      String bu = String((char)(0xF0 | lowByte(i)));
      bu.concat(s.substring(i * payloadLen, (i * payloadLen) + payloadLen));
      sentBytes += (sendString(bu) - 1); 
      delay(4); //20 characters characcters at 9600 baud is about 2.1 msec...make at least 10% longer (if not 2x longer)
  }

  //Serial.print("BLE: sendMessage: sentBytes = "); Serial.println((unsigned int)sentBytes);
  if (s.length() == sentBytes)
      return sentBytes;

  return 0;
}

size_t BLE_nRF52::sendCommand(const String &cmd,const String &data) {
  size_t len = 0;

  //usualy something like cmd="SET NAME=" with data = "MY_NAME"
  //or something like cmd="GET NAME" with data =''
  String total_message = cmd + data;
  len += serialToBLE->write(total_message.c_str(), total_message.length() );   
  //Serial.println("BLE_nRF52:sendCommand: sent " + total_message);
  serialToBLE->write(EOC.c_str(),EOC.length());  // our AT command set on the nRF52 assumes that each command ends in a '\r'
  return len;
}


size_t BLE_nRF52::recvReply(String &reply, unsigned long timeout_millis) {
  unsigned long max_millis = millis() + timeout_millis;
  bool waiting_for_EOC = true;
  bool waiting_for_O_or_F = true;
  while ((millis() < max_millis) && (waiting_for_EOC)) {
    while (serialFromBLE->available()) {
      char c = serialFromBLE->read();
      if (waiting_for_O_or_F) {
        if ((c=='O') || (c=='F')) {
          waiting_for_O_or_F = false;
        }
        if (c != EOC[0]) reply.concat(c);
      } else {
        if (c == EOC[0]) { //this is a cheat!  someone, someday might want a two-character EOC (such as "\n\r"), at which point this will fail
          waiting_for_EOC = false;
        } else {
          reply.concat(c);
        }
      }
    }
  }

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
  reply = reply.trim();

  //Serial.println("BLE_nRF52: recvReply: reply: " + reply);
  return reply.length();
}

bool BLE_nRF52::doesStartWithOK(const String &s) {
  return ((s.length() >= 2) && (s.substring(0,2)).equals("OK"));
}

int BLE_nRF52::setBleName(const String &s) {
  sendCommand("SET NAME=", s);
  //if (simulated_Serial_to_nRF) bleUnitServiceSerial();  //for the nRF firmware, service any messages coming in the serial port from the Tympan
  
  String reply;
  recvReply(reply);
  if (doesStartWithOK(reply)) {  
    return 0;
  } else {
    Serial.println("BLE_nRF52: setBleName: failed to set the BLE name.  Reply = " + reply);
  }
  return -1;
}

int BLE_nRF52::getBleName(String &reply) {
  sendCommand(String("GET NAME"), String(""));
  //if (simulated_Serial_to_nRF) bleUnitServiceSerial();  //for the nRF firmware, service any messages coming in the serial port from the Tympan
  
  recvReply(reply); //look for "OK MY_NAME"
  //Serial.println("BLE_nRF52: getBleName: received raw reply = " + reply);
  if (doesStartWithOK(reply)) {
    reply.remove(0,2);  //remove the leading "OK"
    if ((reply.length() > 0) && (reply[0]==' ')) reply.remove(0,1);  //if present, remove the space that should have been after "OK"
    return 0;
  } else {
    Serial.println("BLE_nRF52: getBleName: failed to get the BLE name.  Reply = " + reply);
    if (reply.length() > 0) reply.remove(0,reply.length());  //empty the string
  }
  return -1;
}


int BLE_nRF52::getLedMode(void) {
  sendCommand("GET LEDMODE",String(""));
  //if (simulated_Serial_to_nRF) bleUnitServiceSerial();  //for the nRF firmware, service any messages coming in the serial port from the Tympan
  
  String reply;
  recvReply(reply); //look for "OK MY_NAME"
  //Serial.println("BLE_nRF52: version: received raw reply = " + reply);
  if (doesStartWithOK(reply)) {
    reply.remove(0,2);  //remove the leading "OK"
    reply.trim(); //remove leading or trailing whitespace
    return int(reply.toInt());
  } 
  return -1; //error
}


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
  recvReply(reply); //look for "OK MY_NAME"
  //Serial.println("BLE_nRF52: version: received raw reply = " + reply);
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
  recvReply(reply); //look for "OK MY_NAME"
  //Serial.println("BLE_nRF52: version: received raw reply = " + reply);
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

int BLE_nRF52::enableAdvertising(bool enable) {
  if (enable) {
    sendCommand("SET ADVERTISING=","ON");
  } else {
    sendCommand("SET ADVERTISING=","OFF");
  }
  String reply;
  recvReply(reply); //look for "OK MY_NAME"
  //Serial.println("BLE_nRF52: enableAdvertising: received raw reply = " + reply);
  if (doesStartWithOK(reply)) {
    return 0;
  }
  return -1; //error
}

int BLE_nRF52::setLedMode(int value) {
  sendCommand("SET LEDMODE=",String(value));

  String reply;
  recvReply(reply); //look for "OK""
  //Serial.println("BLE_nRF52: version: received raw reply = " + reply);
  if (doesStartWithOK(reply)) {
    return 0;
  }
  return -1; //error
}

int BLE_nRF52::version(String &replyToReturn) {
  sendCommand("GET VERSION",String(""));
  //if (simulated_Serial_to_nRF) bleUnitServiceSerial();  //for the nRF firmware, service any messages coming in the serial port from the Tympan
  
  String reply;
  recvReply(reply); //look for "OK MY_NAME"
  //Serial.println("BLE_nRF52: version: received raw reply = " + reply);
  if (doesStartWithOK(reply)) {
    reply.remove(0,2);  //remove the leading "OK"
    reply.trim(); //remove leading or trailing whitespace
    //if (printResult) {
    //  Serial.println("BLE_nR52: version: nRF52 module replied as '" + reply + "'");
    //}
    replyToReturn.remove(0,replyToReturn.length());
    replyToReturn += reply;
    return 0;
  } else {
    Serial.println("BLE_nRF52: version: failed to get the BLE firmware version.  Reply = " + reply);
    if (reply.length() > 0) reply.remove(0,reply.length());  //empty the string
  }  
  return -1;
}





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
	Serial.println(F("   d/D: Activate/Deactivate BT Classic discoverable connectable"));
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
			Serial.println("BLE_nRF52: " + String(c) + " not yet implemented");
			//Serial.println("BLE: activating BLE advertising...");
			//advertise(true);
			break;
		case 'A':
			Serial.println("BLE_nRF52: " + String(c) + " not yet implemented");
			//Serial.println("BLE: de-activating BLE advertising...");
			//advertise(false);
			break;
		case 'd':
			Serial.println("BLE_nRF52: " + String(c) + " not yet implemented");
			//Serial.println("BLE: activating BT Classic discoverable...");
			//if (get_BC127_firmware_ver() >= 7)  {
			//	discoverableConnectableV7(true);
			//} else {
			//	discoverable(true);
			//}
			break;
		case 'D':
			Serial.println("BLE_nRF52: " + String(c) + " not yet implemented");
			//Serial.println("BLE: de-activating BT Classic discoverable...");
			//if (get_BC127_firmware_ver() >= 7)  {
			//	discoverableConnectableV7(false);
			//} else {
			//	discoverable(false);
			//}
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
				BLE_nRF52::version(reply);
				Serial.println("BLE_nRF52: Version = " + reply);
			}
			break;
		default:
			ret_val = false;
		
	}
	return ret_val;
}
