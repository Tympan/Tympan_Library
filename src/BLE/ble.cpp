#include "ble.h"

const int factory_baudrate = 9600;   //here is one possible starting baudrate for the BLE module
const int faster_baudrate = 115200; //here is the other possible starting baudrate for the BLE module


int BLE::begin(int doFactoryReset)  //0 is no, 1 = hardware reset
{
	int ret_val = 0;
	
	//clear the incoming serial buffer
	while (_serialPort->available()) _serialPort->read();  //clear the serial buffer associated with the BT unit
	

	//do a factory reset
	if (doFactoryReset == 1) {
		Serial.println("BLE: begin: doing factory reset via hardware pins.");
		hardwareFactoryReset(); //this also automatically changes the serial baudrate of the Tympan itself

	} else {
		//force into command mode (not needed if already in command mode...but historical Tympan units were preloaded to Data mode instead)
		_serialPort->print("$");  delay(400);	_serialPort->print("$$$");
	}
	
	//switch BC127 to listen to the serial link from the Tympan to a faster baud rate
	if (useFasterBaudRateUponBegin) {
		Serial.println("BLE: begin: setting BC127 baudrate to " + String(faster_baudrate) + ".");
		switchToNewBaudRate(faster_baudrate);
	} else {
		Serial.println("BLE: begin: keeping baudrate at default.");
	}
		
	//enable BT_Classic connectable and discoverable, always
	if (BC127_firmware_ver >= 7) {
		ret_val = setConfig("BT_STATE_CONFIG", "1 1");
		if (ret_val != BC127::SUCCESS) {
			Serial.println(F("BLE: begin: set BT_STATE_CONFIG returned error ") + String(ret_val));
			
			//try a different baud rate
			Serial.println("BLE: begin: switching baudrate to BT module to " + String(faster_baudrate));
			setSerialBaudRate(faster_baudrate);
			delay(100); _serialPort->print(EOC); delay(100); echoBTreply();
			
			//try BT_Classic again
			ret_val = setConfig("BT_STATE_CONFIG", "1 1");
			if (ret_val != BC127::SUCCESS) {
				Serial.println(F("BLE: begin: set BT_STATE_CONFIG returned error ") + String(ret_val));
				
				//switch back to the factory baud rate
				Serial.println("BLE: begin: switching baudrate to BT module back to " + String(factory_baudrate));
				setSerialBaudRate(factory_baudrate);
				delay(100); _serialPort->print(EOC); delay(100); echoBTreply();
			}
		}
	}
	
	//write the new configuration so that it exists on startup (such as an unexpected restart of the module)
    ret_val = writeConfig();
	if (ret_val != BC127::SUCCESS) {
		Serial.println(F("BLE: begin: writeConfig() returned error ") + String(ret_val));
		
		//try a different baud rate
		Serial.println("BLE: begin: switching baudrate to BT module to " + String(faster_baudrate));
		setSerialBaudRate(faster_baudrate);
		delay(100); _serialPort->print(EOC); delay(100); echoBTreply();
		
	    ret_val = writeConfig();
		if (ret_val != BC127::SUCCESS) {
			Serial.println(F("BLE: begin: writeConfig() returned error ") + String(ret_val));
		
			//switch back to the factory baud rate
			Serial.println("BLE: begin: switching baudrate to BT module back to " + String(factory_baudrate));
			setSerialBaudRate(factory_baudrate);
			delay(100); _serialPort->print(EOC); delay(100); echoBTreply();
		}
	}
			
	
	//reset
    ret_val = reset();
	if (ret_val != BC127::SUCCESS)	Serial.println(F("BLE: begin: reset() returned error ") + String(ret_val));

	return ret_val;
}

void BLE::setSerialBaudRate(int new_baud) {
	_serialPort->flush();
	_serialPort->end();
	_serialPort->begin(new_baud);
	delay(100);
}

int BLE::hardwareFactoryReset(bool printDebug) {
	if (printDebug) Serial.println("BLE: hardwareFactoryReset: starting hardware-induced reset...");
	
	//before the hardware reset, change the serial baudrate to the expected new value
	setSerialBaudRate(factory_baudrate);
	
	//do the hardware reset
	int ret_val = factoryResetViaPins();
	if (ret_val < 0) { //factoryResetViaPins is in BC127
		Serial.println("BLE: hardwareFactoryReset: *** ERROR ***: could not do factory reset.");
		return -1;  //error!
	}
	
	//wait for the reset info to be issued by the module over the serial link
	delay(2000);
	echoBTreply(printDebug);

	//check communication ability
	if (printDebug) {
		Serial.println("BLE: hardwareFactoryReset: asking BC127 Status: ");
		status(true); //print version info again.
	}
	
	return 0;  //OK!
}

void BLE::echoBTreply(const bool printDebug) {
	bool if_any_received = false;
	while (_serialPort->available()) { 
		if_any_received = true;
		char c = _serialPort->read(); //clear the character out of the incoming buffer
		//echo the character to the USB serial?
		if (printDebug) Serial.print(c);
	}
	if (printDebug && if_any_received) { if (BC127_firmware_ver >= 7) Serial.println(); }
}

void BLE::switchToNewBaudRate(int new_baudrate) {
	const bool printDebug = false;

	//Send the command to increase the baud rate.  Takes effect immediately
	//Because it takes effect immediately, you can't use the setConfig() function due to setConfig()
	//trying to listen or a reply.  While the BC127 will give a reply, its reply will be at the new
	//baud rate, which will be the wrong baud rate for the Tympan until we swap the Tympan's baud rate
	//to match.  So, we need to manually send the command, then swap the Tympan's baud rate, then listen
	//new replies.
	if (BC127_firmware_ver >= 7) {
		if (printDebug) Serial.println("BLE: switchToNewBaudRate: V7: changing BC127 to " + String(new_baudrate));
		_serialPort->print("SET UART_CONFIG=" + String(new_baudrate) + " OFF 0" + EOC);
	} else {
		if (printDebug) Serial.println("BLE: switchToNewBaudRate: V5: changing BC127 to " + String(new_baudrate));
		_serialPort->print("SET BAUD=" + String(new_baudrate) + EOC); 
	}
	
	//Switch the serial link to the BC127 to the faster baud rate
	setSerialBaudRate(new_baudrate);
	if (printDebug) Serial.println("BLE: switchToNewBaudRate: setting Serial link to BC127 to " + String(new_baudrate));
	
	
	//give time for any replies from the module
	delay(500);   //500 seems to work on V5
	if (printDebug) Serial.println("BLE: switchToNewBaudRate: Reply from BC127 (if any)...");
	echoBTreply(printDebug);
	
	//clear the serial link by sending a CR...will return an error
	if (printDebug) Serial.println("BLE: switchToNewBaudRate: Confirming asking status");	
	_serialPort->print("STATUS" + EOC);  
	delay(100);	
	echoBTreply(printDebug); //should cause response of "ERROR"
}

void BLE::setupBLE(int BT_firmware, bool printDebug) 
{  
	int doFactoryReset = 1;
	setupBLE(BT_firmware, printDebug, doFactoryReset);
}

void BLE::setupBLE_noFactoryReset(int BT_firmware, bool printDebug)
{
	int doFactoryReset = 0;
	setupBLE(BT_firmware, printDebug, doFactoryReset);	
}

void BLE::setupBLE(int BT_firmware, bool printDebug, int doFactoryReset)
{
    int ret_val;
    ret_val = set_BC127_firmware_ver(BT_firmware);
    if (ret_val != BT_firmware) {
        Serial.println("BLE: setupBLE: *** WARNING ***: given BT_firmware (" + String(BT_firmware) + ") not allowed.");
        Serial.println("   : assuming firmware " + String(ret_val) + " instead. Continuing...");
    }
	
    ret_val = begin(doFactoryReset); //via BC127.h, success is a value of 1
    if (ret_val != 1) { 
        Serial.println("BLE: setupBLE: ble did not begin correctly.  error = " + String(ret_val));
        Serial.println("    : -1 = TIMEOUT ERROR");
        Serial.println("    :  0 = GENERIC MODULE ERROR");
    }

	//start the advertising for a connection (whcih will be maintained in serviceBLE())
	advertise(true);

	//print version information...this is for debugging only
	if (printDebug) Serial.println("BLE: setupBLE: assuming BC127 firmware: " + String(BC127_firmware_ver) + ", Actual is:");
	version(printDebug);
}


size_t BLE::sendByte(char c)
{
    //Serial.print("BLE: sendBytle: "); Serial.println(c);
    String s = String("").concat(c);
    if (send(s)) return 1;

    return 0;
}

size_t BLE::sendString(const String &s)
{
    //Serial.print("BLE: sendString: "); Serial.println(s);
    if (send(s)) return s.length();

    return 0;
}

size_t BLE::sendMessage(const String &orig_s)
{
    String s = orig_s;
    const int payloadLen = 19;
    size_t sentBytes = 0;

    String header;
    header = "\xab\xad\xc0\xde"; // ABADCODE, message preamble
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
        //Serial.println("BLE: sendMessage: ***WARNING*** message is being padded with a space to avoid its length being an unallowed value.");
        s.concat(' '); //append a space character

        //regenerate the size-related information for the header
        int lenBytes = (s.length() << 1) | 0x8001; //the 0x8001 is avoid the first message having the 2nd-to-last byte being NULL
        header[5] = ((char)highByte(lenBytes));
        header[6] = ((char)lowByte(lenBytes));
    }

    //Serial.println("BLE: sendMessage: Header (" + String(header.length()) + " bytes): '" + header + "'");
    //Serial.println("BLE: Message: '" + s + "'");

    //send the packet with the header information
    char buf[16];
    sprintf(buf, "%02X %02X %02X %02X %02X %02X %02X", header.charAt(0), header.charAt(1), header.charAt(2), header.charAt(3), header.charAt(4), header.charAt(5), header.charAt(6));
    //Serial.println(buf);
    int a = sendString(header);
    if (a != 7)  {
		//only print if V5 or, if it is V7, if there is a valid connection (noted by a valid BLE_id_num)
		if ((BC127_firmware_ver < 6) || (BLE_id_num > 0)) {
			Serial.println("BLE: sendMessage: Error in sending header... Sent: '" + String(a) + "'");
		}	
		//if we really do get an error, should we really try to transmit all the packets below?  Seems like we shouldn't.
	}
		

    //break up String into packets
    int numPackets = ceil(s.length() / (float)payloadLen);
    for (int i = 0; i < numPackets; i++)  {
        String bu = (char)(0xF0 | lowByte(i));
        bu.concat(s.substring(i * payloadLen, (i * payloadLen) + payloadLen));
        sentBytes += (sendString(bu) - 1);
        delay(4); //20 characters characcters at 9600 baud is about 2.1 msec...make at least 10% longer (if not 2x longer)
    }

    //Serial.print("BLE: sendMessage: sentBytes = "); Serial.println((unsigned int)sentBytes);
    if (s.length() == sentBytes)
        return sentBytes;

    return 0;
}

size_t BLE::recvMessage(String *s)
{
    int msgSize = 0;
    int bytesRecvd = 0;

    while (available() > 0) {

        if (recvBLE(s) > 0) {

            if (s->startsWith("\xab\xad\xc0\xde\xff")) {

                msgSize = word(s->charAt(5), s->charAt(6));
                Serial.println("BLE: recvMessage: Length of message: '" + String(msgSize) + "'");

                char buf[16];
                sprintf(buf, "%02X %02X %02X %02X %02X %02X %02X", s->charAt(0), s->charAt(1), s->charAt(2), s->charAt(3), s->charAt(4), s->charAt(5), s->charAt(6));
                Serial.println(buf);

                int numPackets = ceil(msgSize / 20.0);
                for (int i = 0; i < numPackets; i++) bytesRecvd += recvBLE(s);
                if (bytesRecvd == msgSize) return bytesRecvd;

            }
            continue;
        }
        break;
    }

    return 0;
}

/* size_t BLE::maintainBLE(void) {
	if isConnected() {
		//do nothing
	} else {
		//check to see if advertising

		//
	}
} */

size_t BLE::recvBLE(String *s, bool printResponse)
{
    String tmp = String("");

    // get our start time
    unsigned long startTime = millis();

    // as long as we have time
    while ((startTime + _timeout) > millis())  {
        if (recv(&tmp) > 0)    {
			if (printResponse) Serial.println("BLE: recvBLE: received = " + tmp);
			
			if (BC127_firmware_ver < 7) {
				if (tmp.startsWith("RECV BLE ")) //for V5 firmware for BC127
				{
					s->concat(tmp.substring(9).trim());
					return tmp.substring(9).length();
				}
			} else {
				//if (tmp.startsWith("RECV " + String(BLE_id_num) + " ")) //for V6 and newer...assumes first ("1") BLE link ("4")
				if ( tmp.startsWith("RECV 14 ") || tmp.startsWith("RECV 24 ") | tmp.startsWith("RECV 34 ") ) //for V6 and newer...assumes first ("1") BLE link ("4")
				{
					int new_link_id = (tmp.substring(5,7)).toInt();
					if (new_link_id != BLE_id_num) {
						Serial.println(F("BLE: recvBLE: received 'RECV ") + String(new_link_id) + F("' so we now assume our BLE Link is ") + String(new_link_id));
						BLE_id_num = new_link_id;
					}
					
					tmp.remove(0,8);  tmp.trim();  //remove the "RECV 14 "
					int ind = tmp.indexOf(" ");    //find the space after the number of charcters
					if (ind >= 0) {
						//int len_string = (tmp.substring(0,ind)).toInt();  //it tells you the number of characters received
						tmp.remove(0,ind); tmp.trim(); //remove the number of characters received and any white space
						s->concat(tmp); //what's left is the message
					}
					return tmp.length();
				}
				else {
					interpretAnyOpenOrClosedMsg(tmp,printResponse);
				}
			}
            tmp = "";
        }
    }

    return 0;
}

//returns true if an open or closed message is found
bool BLE::interpretAnyOpenOrClosedMsg(String tmp, bool printDebug) {
	bool ret_val = true;
	
	if (tmp.startsWith("OPEN_OK 14")) {
		BLE_id_num = 14;
		Serial.println(F("BLE: lookForOpenOrClosedMsg: received OPEN_OK for BLE Link ") + String(BLE_id_num));
		
	} else if (tmp.startsWith("OPEN_OK 24")) {
		BLE_id_num = 24;
		Serial.println(F("BLE: lookForOpenOrClosedMsg: received OPEN_OK for BLE Link ") + String(BLE_id_num));
		
	} else if (tmp.startsWith("OPEN_OK 34")) {
		BLE_id_num = 34;
		Serial.println(F("BLE: lookForOpenOrClosedMsg: received OPEN_OK for BLE Link ") + String(BLE_id_num));
		
	} else if (tmp.startsWith("CLOSE_OK " + String(BLE_id_num))) {
		Serial.println(F("BLE: lookForOpenOrClosedMsg: received CLOSE_OK for BLE Link ") + String(BLE_id_num));
		BLE_id_num = -1;
		
	} else {
		ret_val = false;
	}
	
	return ret_val;
	
}


bool BLE::isAdvertising(bool printResponse)
{
    //Ask the BC127 its advertising status.
    //in V5: the reply will be something like: STATE CONNECTED or STATE ADVERTISING
    //in V7: the reply will be something like: STATE CONNECTED[0] CONNECTABLE[OFF] DISCOVERABLE[OFF] BLE[ADVERTISING]
    if (status() > 0) //in bc127.cpp.    answer stored in cmdResponse.
    {
		String s = getCmdResponse();  //gets the text reply from the BC127 due to the status() call above
		
		while ((s.length() >0) && (interpretAnyOpenOrClosedMsg(s,printResponse))) s = getCmdResponse(); //in case the expected message got hijacked


		if (printResponse) {
			Serial.print("BLE: isAdvertising() response: ");
			Serial.print(s);
			if (BC127_firmware_ver > 6) Serial.println();
		}
        //return s.startsWith("STATE CONNECTED"); //original
		if (s.indexOf("ADVERTISING") == -1) { //if it finds -1, then it wasn't found
			//Serial.println("BLE: isAdvertising: not advertising.");
			return false;
		} else {
			
			//Serial.println("BLE: isAdvertising: yes is advertising.");
			return true;
		}
    }

    return false;
}
bool BLE::isConnected(bool printResponse)
{
    //Ask the BC127 its advertising status.
    //in V5: the reply will be something like: STATE CONNECTED or STATE ADVERTISING
    //in V7: the reply will be something like: STATE CONNECTED[0] CONNECTABLE[OFF] DISCOVERABLE[OFF] BLE[ADVERTISING]
    //   followed by LINK 14 CONNECTED or something like that if the BLE is actually connected to something
    if (status() > 0) //in bc127.cpp.  answer stored in cmdResponse.
    {
		String s = getCmdResponse();  //gets the text reply from the BC127 due to the status() call above
		if (printResponse) {
			Serial.print("BLE: isConnected()   response: ");
			Serial.print(s);
			if (BC127_firmware_ver > 6) Serial.println();
		}
        
		//if (s.indexOf("LINK 14 CONNECTED") == -1) { //if it returns -1, then it wasn't found.  This version is prob better (more specific for BLE) but only would work for V6 and above
		int ind = s.indexOf("CONNECTED");
		if (ind == -1) { //if it returns -1, then it wasn't found.
			if (printResponse) Serial.println("BLE: isConnected: not connected.");
			return false;
		} else {
			//as of V6 (or so) it'll actually say "CONNECTED[0]" if not connected, which must be for BT Classic
			//not BLE.  So, instead, let's search for "LINK 14 CONNECTED", which is maybe overly restrictive as
			//it only looks for the first "1" of the possible BLE "4" connections.
			if (BC127_firmware_ver >= 6) {
				ind = s.indexOf("LINK 14 CONNECTED");
				int ind2 = s.indexOf("LINK 24 CONNECTED");
				int ind3 = s.indexOf("LINK 34 CONNECTED");
				int ind4 = s.indexOf("BLE[CONNECTED]");
				if ( (ind == -1) && (ind2 == -1) && (ind3 == -1) && (ind4 == -1) ) { //if none are found, we are not connected
					//no BLE-specific connection message is found.
					//if (printResponse) Serial.println("BLE (v7): isConnected: not connected...");				
					return false;
				} else {
					//if (printResponse) Serial.println("BLE: isConnected: yes is connected.");
					return true;
				}
			} else {
				//for V5.5, here are the kinds of lines that one can see:
				//
				// Here are lines with no connection...BLE is last keywoard: either "ADVERTISING" or "IDLE"
				//  This line has no connections (but everyone is ready):  		STATE CONNECTABLE DISCOVERABLE ADVERTISING
				//  This line has no connections (BLE advertising off):    		STATE CONNECTABLE DISCOVERABLE IDLE
				//  This line has no connections (BT Classic off):         		STATE CONNECTABLE ADVERTISING
				//
				//  BT Classic is connected but BLE is not (nor advertising):	STATE CONNECTED IDLE
				//  BT Classic is connected and BLE is not (but is advertising):STATE CONNECTED ADVERTISING
				//
				//  and here is BLE connected:                             		STATE CONNECTED CONNECTED
				
				//Serial.print("BLE (v5x): ind of 'Connected' = ");
				//Serial.println(ind);
				
				//if we got this far, then at least one CONNECTED is seen.  Let's look for IDLE or ADVERTISING, either of which
				//indicate that it's not BLE that is connected
				ind = s.indexOf("IDLE");
				int ind2 = s.indexOf("ADVERTISING");
				if ( (ind >= 0) || (ind2 >= 0) ) { //if either are found, we are not connected
					//Serial.println("BLE: isConnected: found IDLE or ADVERTISING...so NOT connected.");
					//there is IDLE...so, there is no connection
					return false;
				} else {
					//if (printResponse) Serial.println("BLE: isConnected: yes is connected.");
					return true;
				}
			}
		}
    }

	//if we got this far, let's assume that we are not connected
    return false;
}

bool BLE::waitConnect(int time)
{
    // some output has multiple lines
    String line = String("");

    // decide how long we're willing to wait
    int timeout = time > 0 ? time : _timeout;

    // get our start time
    unsigned long startTime = millis();

    // as long as we have time
    while ((startTime + timeout) > millis())
    {
        if (_serialPort->available() > 0)
        {
            line.concat((char)_serialPort->read());
        }

        if (line.endsWith(EOL))
        {
            if (line.startsWith("OPEN_OK BLE")) //V5.5
            {
                return true;
            }
            if (line.startsWith("OPEN_OK 14 BLE")) //V6 and newer
            {
                return true;
            }

            // move on to next line
            line = "";
        }
    }

    return false;
}

void BLE::updateAdvertising(unsigned long curTime_millis, unsigned long updatePeriod_millis, bool printDebugMsgs) {
  static unsigned long lastUpdate_millis = 0;

	if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock

	//has enough time passed to update everything?
	if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
		if (isConnected(printDebugMsgs) == false) { //the true tells it to print the full reply to the serial monitor
			if (isAdvertising(printDebugMsgs) == false) {//the true tells it to print the full reply to the serial monitor
				Serial.println("BLE: updateAvertising: activating BLE advertising");
				advertise(true);  //not connected, ensure that we are advertising
			}
		}
		lastUpdate_millis = curTime_millis; //we will use this value the next time around.
	}	
}



// /////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// UI Methods
//
// 
// /////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BLE_UI::printHelp(void) {
	String prefix = getPrefix();  //getPrefix() is in SerialManager_UI.h, unless it is over-ridden in this class somewhere
	Serial.println(F(" BLE: Prefix = ") + prefix);
	Serial.println(F("   s:   Print Bluetooth status"));
	Serial.println(F("   v:   Print Bluetooth Firmware version info"));
	Serial.println(F("   d/D: Activate/Deactivate BT Classic discoverable connectable"));
	Serial.println(F("   a/A: Activate/Deactivate BLE advertising"));
}


bool BLE_UI::processCharacterTriple(char mode_char, char chan_char, char data_char) {
	//check the mode_char to see if it corresponds with this instance of this class.  If not, return with no action.
	if (mode_char != ID_char) return false;  //ID_char is from SerialManager_UI.h
	
	return processSingleCharacter(data_char);
}

//respond to serial commands
bool BLE_UI::processSingleCharacter(char c) {
  
	bool ret_val = true;
	switch (c) {
		case 'a':
			Serial.println("BLE: activating BLE advertising...");
			advertise(true);
			break;
		case 'A':
			Serial.println("BLE: de-activating BLE advertising...");
			advertise(false);
			break;
		case 'd':
			Serial.println("BLE: activating BT Classic discoverable...");
			if (get_BC127_firmware_ver() >= 7)  {
				discoverableConnectableV7(true);
			} else {
				discoverable(true);
			}
			break;
		case 'D':
			Serial.println("BLE: de-activating BT Classic discoverable...");
			if (get_BC127_firmware_ver() >= 7)  {
				discoverableConnectableV7(false);
			} else {
				discoverable(false);
			}
			break;     
		case 's':
			Serial.println("BLE: printing Bluetooth status...");
			status(true);
			break;     
		case 'v':
			Serial.println("BLE: printing Bluetooth firmware version info...");
			version(true);
			break;
		default:
			ret_val = false;
		
	}
	return ret_val;
}
