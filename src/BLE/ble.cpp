#include "ble.h"

//BLE::BLE(Stream *sp) : BC127(sp)
//{
//
//}

int BLE::begin(void)
{
	//force into command mode (not needed if already in command mode...but historical Tympan units were preloaded to Data mode instead)
	//myTympan.forceBTtoDataMode(false);
	_serialPort->print("$");  delay(400);	_serialPort->print("$$$");

	//now do the usual setup stuff
	int ret_val;
	ret_val = restore();
	if (ret_val != BC127::SUCCESS) {
		Serial.print(F("BLE: begin: restore() returned error "));
		Serial.println(ret_val);
		//return ret_val;
	}
    ret_val = writeConfig();
	if (ret_val != BC127::SUCCESS) {
		Serial.print(F("BLE: begin: writeConfig() returned error "));
		Serial.println(ret_val);
		//return ret_val;
	}	
    ret_val = reset();
	if (ret_val != BC127::SUCCESS) {
		Serial.print(F("BLE: begin: reset() returned error "));
		Serial.println(ret_val);
		//return ret_val;
	}		
	
	return ret_val;
}

void BLE::setupBLE(Tympan &_tympan) {

  _tympan.forceBTtoDataMode(false); //for BLE, it needs to be in command mode, not data mode
  
  int ret_val = begin();
  if (ret_val != 1) {  //via BC127.h, success is a value of 1
    Serial.print("BLE: setupBLE: ble did not begin correctly.  error = ");  Serial.println(ret_val);
    Serial.println("    : -1 = TIMEOUT ERROR");
    Serial.println("    :  0 = GENERIC MODULE ERROR");
  }

  //start the advertising for a connection (whcih will be maintained in serviceBLE())
  advertise(true);

}

size_t BLE::sendByte(char c)
{
	//Serial.print("BLE: sendBytle: "); Serial.println(c);
	
    String s = String("").concat(c);
    if (send(s))
        return 1;

    return 0;
}

size_t BLE::sendString(const String &s)
{
	//Serial.print("BLE: sendString: "); Serial.println(s);
	
    if (send(s))
        return s.length();

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
    if (s.length() >= (0x4000 - 1)) {  //we might have to add a byte later, so call subtract one from the actual limit
        Serial.println("BLE: Message is too long!!! Aborting.");
        return 0;
    }
    int lenBytes = (s.length()<<1) | 0x8001; //the 0x8001 is avoid the first message having the 2nd-to-last byte being NULL
    header.concat((char)highByte(lenBytes));
    header.concat((char )lowByte(lenBytes));
	
	//check to ensure that there isn't a NULL or a CR in this header
	if ((header[6] == '\r') || (header[6] == '\0')) {
		//add a character to the end to avoid an unallowed hex code code in the header
		//Serial.println("BLE: sendMessage: ***WARNING*** message is being padded with a space to avoid its length being an unallowed value.");
		s.concat(' ');  //append a space character
		
		//regenerate the size-related information for the header
		int lenBytes = (s.length()<<1) | 0x8001; //the 0x8001 is avoid the first message having the 2nd-to-last byte being NULL
		header[5] = ((char)highByte(lenBytes));
		header[6] = ((char )lowByte(lenBytes));
	}

    //Serial.println("BLE: sendMessage: Header (" + String(header.length()) + " bytes): '" + header + "'");
    //Serial.println("BLE: Message: '" + s + "'");

	//send the packet with the header information
    char buf[16];
    sprintf(buf, "%02X %02X %02X %02X %02X %02X %02X", header.charAt(0), header.charAt(1), header.charAt(2), header.charAt(3), header.charAt(4), header.charAt(5), header.charAt(6));
    //Serial.println(buf);
    int a = sendString(header);
    if (a != 7) Serial.println("BLE: sendMessage: Error in sending header... Sent: '" + String(a) + "'");


/* 	int ind_start = 0, ind_end=0, ind_out;
	char bu[1+payloadLen+1];  //temporary buffer
	int packet_counter = 0;

	String foo_s;
	while (ind_end < len) {
		//compute indices into our source string
		ind_start = ind_end;
		ind_end = ind_start + payloadLen;
		ind_end = min(ind_end,len);
		
		//construct this payload
		ind_out = 0;
		bu[ind_out++] = (char)(0xF0 | lowByte(packet_counter++));  //first byte
		while (ind_start < ind_end) {
			bu[ind_out++]=c_str[ind_start++];  //payload
		}
		bu[ind_out] = '\0';  //trailing byte...null terminated c-style string
		
		//send the payload
		//sentBytes += (sendString(String(bu))-1);
		foo_s = String(bu);
		size_t foo = sendString(foo_s);
		if (foo > 0) sentBytes += (foo-1);
		//Serial.println("BLE: sendMessage: packet " + String(packet_counter) + ", " + String(ind_start) + ", " + String(ind_end) + ", sentBytes = " + String(foo) + ", " + foo_s);
		//delay(10);
		delay(5); //20 characters characcters at 9600 baud is about 2.1 msec...make at least 10% longer (if not 2x longer)
	} */


	//break up String into packets
    int numPackets = ceil(s.length() / (float)payloadLen);
	for (int i = 0; i < numPackets; i++)
    {
        String bu = (char)(0xF0 | lowByte(i));
        bu.concat(s.substring(i * payloadLen, (i * payloadLen) + payloadLen));
        sentBytes += (sendString(bu)-1);
        delay(4); //20 characters characcters at 9600 baud is about 2.1 msec...make at least 10% longer (if not 2x longer)
    }

	//Serial.print("BLE: sendMessage: sentBytes = "); Serial.println((unsigned int)sentBytes);
    if (s.length() == sentBytes) return sentBytes;

    return 0;
}

size_t BLE::recvMessage(String *s)
{
    int msgSize = 0;
    int bytesRecvd = 0;

    while (1)
    {
        if (available() > 0)
        {
            if (recvBLE(s) > 0)
            {
                if (s->startsWith("\xab\xad\xc0\xde\xff"))
                {
                    msgSize = word(s->charAt(5), s->charAt(6));
                    Serial.println("BLE: recvMessage: Length of message: '" + String(msgSize) + "'");

                    char buf[16];

                    sprintf(buf, "%02X %02X %02X %02X %02X %02X %02X", s->charAt(0), s->charAt(1), s->charAt(2), s->charAt(3), s->charAt(4), s->charAt(5), s->charAt(6));

                    Serial.println(buf);

                    int numPackets = ceil(msgSize / 20.0);

                    for (int i = 0; i < numPackets; i++)
                    {
                        bytesRecvd += recvBLE(s);
                    }

                    if (bytesRecvd == msgSize)
                    {
                        return bytesRecvd;
                    }
                }
                continue;
            }

            break;
        }
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

size_t BLE::recvBLE(String *s)
{
    String tmp = String("");

    // get our start time
    unsigned long startTime = millis();

    // as long as we have time
    while ((startTime + _timeout) > millis())
    {
        if (recv(&tmp) > 0)
        {
            if (tmp.startsWith("RECV BLE ")) //for V5 firmware for BC127
            {
                s->concat(tmp.substring(9).trim());
                return tmp.substring(9).length();
            }
			else if (tmp.startsWith("RECV 14 "))  //for V6 and newer...assumes first ("1") BLE link ("4")
			{
				tmp.remove(0,8);  tmp.trim();  //remove the "RECV 14 "
				int ind = tmp.indexOf(" ");    //find the space after the number of charcters
				if (ind >= 0) {
					//int len_string = (tmp.substring(0,ind)).toInt();  //it tells you the number of characters received
					tmp.remove(0,ind); tmp.trim(); //remove the number of characters received and any white space
					s->concat(tmp); //what's left is the message
				}
                return tmp.length();
			}

            tmp = "";
        }
    }

    return 0;
}

bool BLE::isAdvertising(bool printResponse)
{
	//Ask the BC127 its advertising status. 
	//in V5: the reply will be something like: STATE CONNECTED or STATE ADVERTISING
	//in V7: the reply will be something like: STATE CONNECTED[0] CONNECTABLE[OFF] DISCOVERABLE[OFF] BLE[ADVERTISING]
    if (status() > 0) //in bc127.cpp.    answer stored in cmdResponse.
    {
		String s = getCmdResponse();  //gets the text reply from the BC127 due to the status() call above
		if (printResponse) {
			Serial.println("BLE: isAdvertising() response: ");
			Serial.print(s);
			Serial.println();
		}
        //return s.startsWith("STATE CONNECTED"); //original
		if (s.indexOf("ADVERTISING") == -1) { //if it finds -1, then it wasn't found
			return false;
		} else {
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
			Serial.println("BLE: isConnected() response: ");
			Serial.print(s);
			Serial.println();
		}
        
		//if (s.indexOf("LINK 14 CONNECTED") == -1) { //if it returns -1, then it wasn't found.  This version is prob better (more specific for BLE) but only would work for V6 and above
		int ind = s.indexOf("CONNECTED");
		if (ind == -1) { //if it returns -1, then it wasn't found.
			return false;
		} else {
			//as of V6 (or so) it'll actually say "CONNECTED[0]" if not connected, which is not helpful
			//search instead for "LINK 14 CONNECTED", which is maybe overly restrictive as it only looks
			//for the first "1" of the possible BLE "4" connections.
			if (BC127_firmware_ver >= 6) {
				ind = s.indexOf("LINK 14 CONNECTED");
				if (ind == -1) {
					Serial.println("BLE: isConnected: not connected.");
					return false;
				}
			}
			return true;
		}
    }

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

void BLE::updateAdvertising(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    if (isConnected(true) == false) { //the true tells it to print the full reply to the serial monitor
      if (isAdvertising(true) == false) {//the true tells it to print the full reply to the serial monitor
        Serial.println("BLE: updateAvertising: activating BLE advertising");
        advertise(true);  //not connected, ensure that we are advertising
      }
    }
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }	
}
