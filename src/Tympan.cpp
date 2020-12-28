
#include "Tympan.h"

const int TympanBase::BT_uint8_buff_len;

void TympanBase::setupPins(const TympanPins &_pins) {
	AudioControlAIC3206::setResetPin(_pins.resetAIC);
	pins = _pins; //shallow copy to local version
	BT_mode = pins.default_BT_mode;

	//Serial.print("TympanBase: setupPins: pins.potentiometer, given / act: ");
	//Serial.print(_pins.potentiometer); Serial.print(" / "); Serial.println(pins.potentiometer);

	//setup the CCP related pins
	//if (pins.CCP_enable28V != NOT_A_FEATURE) { pinMode(pins.CCP_enable28V,OUTPUT);  setCCPEnable28V(LOW); }
	//if (pins.CCP_atten1 != NOT_A_FEATURE) { pinMode(pins.CCP_atten1,OUTPUT); digitalWrite(pins.CCP_atten1,LOW); }
	//if (pins.CCP_atten2 != NOT_A_FEATURE) { pinMode(pins.CCP_atten2,OUTPUT); digitalWrite(pins.CCP_atten2,LOW); }
	//if (pins.CCP_bigLED != NOT_A_FEATURE) { pinMode(pins.CCP_bigLED,OUTPUT); digitalWrite(pins.CCP_bigLED,LOW); }
	//if (pins.CCP_littleLED != NOT_A_FEATURE) { pinMode(pins.CCP_littleLED,OUTPUT); digitalWrite(pins.CCP_littleLED,LOW); }
	
	//setup the Tympan pins]
	pinMode(pins.potentiometer,INPUT);
	pinMode(pins.amberLED,OUTPUT); digitalWrite(pins.amberLED,LOW);
	pinMode(pins.redLED,OUTPUT); digitalWrite(pins.redLED,LOW);
	if (pins.enableStereoExtMicBias != NOT_A_FEATURE) {
		pinMode(pins.enableStereoExtMicBias,OUTPUT);
		setEnableStereoExtMicBias(true); //enable stereo external mics (REV_D)
	}
	
	//setup the AIC shield pins
	//if (pins.AIC_Shield_enableStereoExtMicBias != NOT_A_FEATURE) {
	//	pinMode(pins.AIC_Shield_enableStereoExtMicBias,OUTPUT);
	//	setEnableStereoExtMicBiasAIC(true); //enable stereo external mics (REV_D)
	//}
	

	//get the comm pins and setup the regen and reset pins
	USB_Serial = pins.getUSBSerial();
	BT_Serial = pins.getBTSerial();
	if (pins.BT_REGEN != NOT_A_FEATURE) {  //for BC127 BT module (RevD and RevE)
		//The BC127 manual says that the REGEN pin should start LOW when power is applied.
		//Surely, by the time that this code is executed, power is already applied, so I
		//don't know what good this does
		pinMode(pins.BT_REGEN,OUTPUT);digitalWrite(pins.BT_REGEN, LOW);
		
		//Then, the BC127 manual says that we should wait 5 msec after appliation of battery voltage
		//to raise REGEN to HIGH to enable the rest of the booting.  Surely, 5 msec has already
		//passed by the time that we get here, but let's delay anyway, just to be sure.
		delay(10);
		//Now, raise REGEN to enable the rest of the booting
		digitalWrite(pins.BT_REGEN,HIGH); //pull high for normal operation
		
		//wait for booting to finish (how long?!?) and lower REGEN to its normally-low position
		delay(100);
		digitalWrite(pins.BT_REGEN,LOW); //then return low
	}
	if (pins.BT_nReset != NOT_A_FEATURE) {  //For RN51 and  BC127 modules.  (RevC, RevD, RevE)
		pinMode(pins.BT_nReset,OUTPUT);
		digitalWrite(pins.BT_nReset,LOW);delay(50); //reset the device  (RevC used only 10 here, not 50.  Is 50 OK for RevC?)
		digitalWrite(pins.BT_nReset,HIGH);  //normal operation.
	}
	if (pins.BT_PIO0 != NOT_A_FEATURE) {
		pinMode(pins.BT_PIO0,INPUT);
	}

	forceBTtoDataMode(true);
};

//void TympanBase::forceBTtoDataMode(bool state) {
//	if (pins.BT_PIO4 != NOT_A_FEATURE) {
//		if (state == true) {
//			pinMode(pins.BT_PIO4,OUTPUT);
//			digitalWrite(pins.BT_PIO4,HIGH);
//		} else {
//			//pinMode(pins.BT_PIO4,INPUT);  //go high-impedance (ie disable this pin)
//			pinMode(pins.BT_PIO4,OUTPUT);
//			digitalWrite(pins.BT_PIO4,LOW);
//		}
//	}
//}
void TympanBase::forceBTtoDataMode(bool state) {
	if (pins.BT_PIO5 != NOT_A_FEATURE) {
		if (state == true) {
			pinMode(pins.BT_PIO5,OUTPUT);
			Serial.println("Tympan: forceBTtoDataMode: Setting PIO5 to HIGH");
			digitalWrite(pins.BT_PIO5,HIGH);
		} else {
			//pinMode(pins.BT_PIO4,INPUT);  //go high-impedance (ie disable this pin)
			pinMode(pins.BT_PIO5,OUTPUT);
			Serial.println("Tympan: forceBTtoDataMode: Setting PIO5 to LOW");
			digitalWrite(pins.BT_PIO5,LOW);
		}
	} else {
		Serial.println("Tympan: forceBTtoDataMode: No PIO5 specified. Cannot force DATA MODE.");
	}
}


int TympanBase::setEnableStereoExtMicBias(int new_state) {
	if (pins.enableStereoExtMicBias != NOT_A_FEATURE) {
		digitalWrite(pins.enableStereoExtMicBias,new_state);
		return new_state;
	} else {
		return pins.enableStereoExtMicBias;
	}
}

/*
int TympanBase::setEnableStereoExtMicBiasAIC(int new_state) {  //for AIC Shield
	if (pins.AIC_Shield_enableStereoExtMicBias != NOT_A_FEATURE) {
		digitalWrite(pins.AIC_Shield_enableStereoExtMicBias,new_state);
		return new_state;
	} else {
		return pins.AIC_Shield_enableStereoExtMicBias;
	}
}
*/	

void TympanBase::beginBluetoothSerial(int BT_speed) {
	BT_Serial->begin(BT_speed);

	switch (getTympanRev()) {
		case (TympanRev::D) : case (TympanRev::D0) : case (TympanRev::D1) : case (TympanRev::D2) : case (TympanRev::D3) : case (TympanRev::E) : case (TympanRev::E1) :
			clearAndConfigureBTSerialRevD();
			break;
		default:
			delay(50);
			break;
	}
}

void TympanBase::clearAndConfigureBTSerialRevD(void) {
   //clear out any text that is waiting
	delay(500);  //do this to wait for BT serial to come alive upon startup.  Is there a bitter way?
	while(BT_Serial->available()) { BT_Serial->read(); delay(20); }

	if (0) { //sometimes have to temporarily disable for RevE development...11/20/2020, WEA
		//transition to data mode
		//Serial.println("Transition BT to data mode");
		BT_Serial->print("ENTER_DATA");BT_Serial->write(0x0D); //enter data mode (Firmware v5 only!).  Finish with carriage return.
		//BT_Serial->print("ENTER_DATA_MODE");BT_Serial->write(0x0D); //enter data mode (Firmware v6 only!).  Finish with carriage return.  Only works if there is an active BT connection to a phone or whatever...so, bascially, it behaves nothing like v5.  :(
		delay(100);
		int count = 0;
		while ((count < 3) & (BT_Serial->available())) { //we should receive on "OK"
		  //Serial.print((char)BT_SERIAL.read());
		  BT_Serial->read(); count++;  delay(5);
		}
		//Serial.println("BT Should be ready.");
	}
}

void TympanBase::setBTAudioVolume(int vol) {  //only works when you are connected via Bluetooth!!!!
	//vol is 0 (min) to 15 (max)
	if (pins.tympanRev >= TYMPAN_REV_D0) {   //This is only for the BC127 BT module
		//get into command mode
		USB_Serial->println("*** Changing BT into command mode...");
		forceBTtoDataMode(false); //un-forcing (via hardware pin) the BT device to be in data mode
		delay(500);
		BT_Serial->print("$");  delay(400);
		BT_Serial->print("$$$");  delay(400);
		delay(2000);
		echoIncomingBTSerial();

		// clear the buffer by forcing an error
		USB_Serial->println("*** Clearing buffers.  Sending carraige return.");
		BT_Serial->print('\r'); delay(500);
		echoIncomingBTSerial();
		USB_Serial->println("*** Should have gotten 'ERROR', which is fine here.");

		//check A2DP volume
		USB_Serial->println("*** Check A2DP and HFP Volume...");
		BT_Serial->print("VOLUME A2DP"); BT_Serial->print('\r');
		delay(1000);
		echoIncomingBTSerial();
		BT_Serial->print("VOLUME HFP"); BT_Serial->print('\r');
		delay(1000);
		echoIncomingBTSerial();

		//change volume
		USB_Serial->println("*** Setting A2DP Volume...");
		BT_Serial->print("VOLUME A2DP="); BT_Serial->print(vol); BT_Serial->print('\r');
		delay(1000);
		echoIncomingBTSerial();
		BT_Serial->print("VOLUME HFP="); BT_Serial->print(vol); BT_Serial->print('\r');
		delay(1000);
		echoIncomingBTSerial();

		//check A2DP volume again
		USB_Serial->println("*** Check A2DP and HFP Volume again...");
		BT_Serial->print("VOLUME A2DP"); BT_Serial->print('\r');
		delay(1000);
		echoIncomingBTSerial();
		BT_Serial->print("VOLUME HFP"); BT_Serial->print('\r');
		delay(1000);
		echoIncomingBTSerial();

		//confirm GPIO State
		USB_Serial->println("*** Setting GPIOCONTROL Mode...");
		BT_Serial->print("SET GPIOCONTROL=OFF");BT_Serial->print('\r'); delay(500);
		echoIncomingBTSerial();

		//save
		USB_Serial->println("*** Saving Settings...");
		BT_Serial->print("WRITE"); BT_Serial->print('\r'); delay(500);

		USB_Serial->println("*** Changing into transparanet data mode...");
		BT_Serial->print("ENTER_DATA");BT_Serial->print('\r'); delay(500);
		echoIncomingBTSerial();
		forceBTtoDataMode(true); //forcing (via hardware pin) the BT device to be in data mode
	} else {
		//need code for other models of Tympan
	}
}


size_t TympanBase::USB_serial_write(uint8_t foo) {
	//print to USB Serial
	size_t count = 0;
	if (USB_dtr()) count += USB_Serial->write(foo); //the USB Serial can jam up, so make sure that something is open on the PC side		
	return count;
}
		
 size_t TympanBase::BT_serial_write(uint8_t foo) {
	//print to BT Serial, using a method that depends upon the BT mode
	if (BT_mode == TympanPins::BT_modes::BT_COMMAND_MODE) {
		if (digitalRead(pins.BT_PIO0) == HIGH) { //this means that there is a connection
			//USB_Serial->println("*** Printing to BT: SEND_RAW 15 1");
			BT_Serial->print("SEND_RAW 15 1");BT_Serial->print('\r'); // The BC127 command is "SEND_RAW".  The "15" is the link_id that appears for SPP
			//USB_Serial->print("*** Printing to BT: one value: "); USB_Serial->println(foo);
			return BT_Serial->write(foo); //write the value and return
		} else {
			return 0;
		}
	} else {
		return BT_Serial->write(foo); //write the value and return
	}
	return 0;  // should never get here
}

 size_t TympanBase::USB_serial_write(const uint8_t *buffer, size_t orig_size) {
	//write to the USB Serial
	const uint8_t *p_buff = buffer;
	size_t count=0;
	size_t size = orig_size;
	if (USB_dtr()) while (size--) count += USB_Serial->write(*p_buff++);
	return count;
}

size_t TympanBase::BT_serial_write(const uint8_t *buffer, size_t orig_size) {	
	size_t count = 0;
	
	//Now write to the BT Serial...the method that we use depends upon the BT mode
	if (BT_mode == TympanPins::BT_modes::BT_COMMAND_MODE) {
		//we're in COMMAND_MODE, so the writing is bit more complicated
		if (digitalRead(pins.BT_PIO0) == HIGH) { //this means that there is a connection
			count = write_BC127_V7_command_mode(buffer,orig_size);
		}
	} else {
		//we're in DATA_MODE, so no preable is necessary.  Just write the bytes.
		size_t size = orig_size;
		const uint8_t *p_buff = buffer;
		while (size--) count += BT_Serial->write(*p_buff++); //write the actual bytes
	}
	return count;
}

int TympanBase::BT_serial_available(void) { 
	if (BT_mode == TympanPins::BT_modes::BT_COMMAND_MODE) {
		//COMMAND_MODE is complicated.  We have to look to see if there are already-buffered values.  If not,
		//we then have to look to see if the BT_Serial has any bytes.  If there are, we have to read those bytes
		//and see if it's a valid data message
		
		//first, check the existing buffer
		int d_ind = (BT_uint8_buff_end-BT_uint8_buff_ind);
		if (d_ind > 0) {
			Serial.print("BT_serial_read, d_ind ="); Serial.println(d_ind);
			return d_ind;
		}
		
		//second check to see if there are bytes on the serial link and, if so, parse them
		if (BT_Serial->available()) {
			//Serial.print("BT_serial_read, BT_Serial->available() ="); Serial.println(BT_Serial->available());
			read_BC127_V7_command_mode();
		}
		
		//check to see if there is any new data to available
		d_ind = (BT_uint8_buff_end-BT_uint8_buff_ind);
		return d_ind;				
		
	} else {
		//Here, we are in DATA_MODE, so it's simply a transparent data link.
		return (BT_Serial->available());
	}
	return 0;  //we should never get here
}

int TympanBase::BT_serial_read(void) { 
	if (BT_mode == TympanPins::BT_modes::BT_COMMAND_MODE) {
		//In COMMAND_MODE, the operation is more complicated.
		
		//First, see if there is any data that we can return that is already in our local buffer
		int d_ind = (BT_uint8_buff_end-BT_uint8_buff_ind);
		if (d_ind > 0) {
			// Send the next character from the local buffer
			return BT_uint8_buff[BT_uint8_buff_ind++];
		} else {
			//There is no data in the lcoal buffer.  So, let's see if there's data in the Serial buffer.
			//If so, move that data into the local buffer.
			read_BC127_V7_command_mode();
			
			//check to see if data has been moved into the local buffer.
			int d_ind = (BT_uint8_buff_end-BT_uint8_buff_ind);
			if (d_ind > 0) {
				//yes, data has been moved into the local buffer. 
				return BT_uint8_buff[BT_uint8_buff_ind++]; 	// Send the next character from the buffer
			} else {
				//There is no data.  So, do nothing here and then we'll the default case at the end of this method
			}
		}
	} else {
		//In DATA_MODE, the link is transparaent, so we can simply ask the Serial port if any data is available
		return BT_Serial->read(); 
	}
	//default, if no other line returns any data
	return -1;  //nothing to return.  I belive that sending -1 is what the Arduino Serial.read() does if there is no data
}

size_t TympanBase::write_BC127_V7_command_mode(const uint8_t *buffer, size_t size) {
	//If in command_mode, we need to send a pre-amble with the command name for send, as well as other info.
	//For the BC127, the command that we will use is "SEND_RAW"
	size_t count = 0;
	
	//Serial.println();Serial.print("write_BC127_V7_command_mode multi: size = "); Serial.println(size);
	//if (size < 1000) { Serial.print("*** buffer to write: ");Serial.write(buffer,size); Serial.println(); }
	
	while (size > 0) {
		//SEND_RAW has a 255 byte limit.  So, if we've got more than 255 to send, we can only send 255 at a time
		if (size > 255) { 
			BT_Serial->print("SEND_RAW 15 255"); BT_Serial->print('\r');  //the "15" is the link_id for SPP.  The 255 is the number of bytes that will be sent.
			for (int i=0; i< 255; i++) { count += BT_Serial->write(*buffer++); }  //write the bytes
			size = size - 255;  //update the counters and remaining size
		} else {  //we've got less than 255 to send, so we'll send whatever bytes we hav eleft to send
			BT_Serial->print("SEND_RAW 15 ");BT_Serial->print(size); BT_Serial->print('\r');
			for (int i=0; i< (int)size; i++) { count += BT_Serial->write(*buffer++); }  //write the bytes
			size = 0;
		}
	}
	return count;
}

void TympanBase::read_BC127_V7_command_mode(void) { //only call this when the read buffer has been emptied

	//make sure that the previous data has been cleared out
	if (BT_uint8_buff_ind < BT_uint8_buff_end) return;  //There is still data in the local buffer, so don't yet pull data from the serial buffer
		
	//prepare to step through the incoming byte stream
	static int state = 0;
	static bool is_bytestream_done = false;
	char targ_str[] = "RECV 15 ";
	int n_targ_str = 8; //number of characters in targ_str
	static char size_str[6];  //for Melody v7 largest is 65535
	static int size_str_ind = 0;
	static int n_bytes_to_read = 0;
	static char ascii_hex[2];
	static int read_ascii_as_hex = 0;
	static int bytes_read_in_this_payload = 0;

	const int STATE__PARSE_SIZE = n_targ_str;
	const int STATE__PARSE_DATA = STATE__PARSE_SIZE+1;
	const int STATE__REJECT = -999;
	
	//Check to see if all the data has been read from the central buffer
	if (BT_uint8_buff_ind == BT_uint8_buff_end) {
		//yes, the data has all been read.  Reset the buffer.
		BT_uint8_buff_ind = 0;
		BT_uint8_buff_end = 0;
	}
	
	//now, loop through the incoming byte stream
	while ( (BT_Serial->available() > 0)   //is there data
			 && (!is_bytestream_done)  //are we done parsing this particular message (even if more data is in the stream)
			 && (BT_uint8_buff_end < BT_uint8_buff_len) ) {  //and is there room to store it in our buffer, if there is data

		//get the character from the serial stream
		uint8_t serial_val = BT_Serial->read();
		char c = (char) serial_val;
		
		//Serial.println(); Serial.print("read_BC127_V7_command_mode: state = "); Serial.print(state); Serial.print(", char = "); Serial.println(c);
	
		//paradoxically, let's start by checking to see if it is the END of the message, which would be a '\r', which might  be expected or unexpected.
		if (state != STATE__PARSE_DATA) {  //during the actual data stream, a '\r' might be present in the bytes stream, but otherwise not.
			//look for carriage return.  Assume that is the end of this message
			if (c == '\r') {
				state = 0; size_str_ind = 0; n_bytes_to_read = 0; //reset
				return;	
			}						
		}

		//now step through each state and see if it agrees with what we expect
		if (state == 0) {
			//wait until we get a match with the first letter. 
			if (c == targ_str[0]) state++;
			
		} else if (state < n_targ_str) {  //we should now be getting all the other letters, in order, no other stuff
			//once we had a match with the first letter, check all the rest of the letters.
			//if we don't get the letter that we expect, go to "STATE__REJECT", which rejects everything until the next carraige return
			if (c == targ_str[state]) {
				//This character is was what we expected, so move on to the next character!
				state++;  //stepping through the target string to see if we've found it!
				//eventually, state++ increments up to equal STATE__PARSE_SIZE, which is the next step in the process
			} else {
				//This character was not what we expected, so switching to STATE__REJECT
				state = 0; size_str_ind = 0; n_bytes_to_read = 0; //reset
				state = STATE__REJECT;
			}
			
		} else if (state == STATE__PARSE_SIZE) {

			//The STATE__PARSE_SIZE must parse a multi-character segment of the data stream.
			//It should be an ascii number (up to 3 digits) followed by a space character
			if (c == ' ') { //look for the terminating character in this phase (which is a space character)
				//We've recevied the terminating character, so go ahead and interepret the ascii string as a number
				size_str[size_str_ind++] = '\0';  //null terminate string (needed for atoi() to function)
				n_bytes_to_read = atoi(size_str); //interpret the ascii string as an integer
				if (n_bytes_to_read > 65535) { 
					Serial.println("Tympan: read_BC127_V7_command_mode: *** WARNING ***");
					Serial.print("    : commanded to read "); Serial.println(n_bytes_to_read);
					Serial.print("    : whereas the limit should be only 65535.  Continuing...");
				}
				size_str_ind = 0;  //reset
				//Serial.print("read_BC127_V7_command_mode: number of bytes in payload = "); Serial.println(n_bytes_to_read);
				
				//move on to the next state of this parser, which is to start reading the data bytes
				BT_uint8_buff_ind = 0; BT_uint8_buff_end = 0;  is_bytestream_done = false; //reset the internal data buffer
				state = STATE__PARSE_DATA; bytes_read_in_this_payload = 0;
				
				
				
			} else if ( (c >= '0') && (c <= '9') ) {
				//We received characters for the ascii number.  Good.
				size_str[size_str_ind++] = c;
				if (size_str_ind == 6) {  //Melody v7 says can be up to 65535, so if we get a 6th digit, it's too many!
					//we have too many numbers?!?  Bail out!  Reject!
					state = STATE__REJECT;
					size_str_ind = 0;
				}
			} else {
				//this is an unexpected character, so ignore everything
				state = STATE__REJECT;
				size_str_ind = 0;
			}
			
		} else if (state == STATE__PARSE_DATA) {
			//Serial.print("Tympan: read_BC127_V7_command_mode: PARSE_DATA: read_ascii_as_hex = ");Serial.println(read_ascii_as_hex);
			if (read_ascii_as_hex==0) {
				//We are in the byte collection phase.  
				
				//Serial.print("Tympan: read_BC127_V7_command_mode: c = "); Serial.print(c); 
				//Serial.print(" , Val = ");Serial.print((int)c);Serial.println();
				
				//first, check to make sure it isn't an escape character
				if (c == '\\') {
					//This is the start of an escape sequence.
					//The next two digits will be a hex code
					read_ascii_as_hex = 2;  //any non-zero number means that it'll try to interpret the next digits as the hex				
				} else {
					//It's not an escape character, to store the byte in the local buffer
					BT_uint8_buff[BT_uint8_buff_end++] = serial_val;
					bytes_read_in_this_payload++;
				}
				
			} else {
				//Serial.print("Tympan: read_BC127_V7_command_mode: reading as hex, c = ");Serial.println(c);
				
				//in this branch, we're reading the characters as an escape sequency (2-digit hex)
				if ( ((c >= '0') && (c <= '9')) ||   //is it a base 10 numeral
					 ((c >= 'A') && ( c <= 'F' )) ) {  //or is it the extend hex numerals
											
					//this is a valid value.  great.
					read_ascii_as_hex--;  //count downward so that we finish at zero
					ascii_hex[1-read_ascii_as_hex] = c; //put into the char array
					
					//Serial.print("Tympan: read_BC127_V7_command_mode: hex saved = "); Serial.println(ascii_hex[1-read_ascii_as_hex]);
					
					//have we gathered both digits?
					if (read_ascii_as_hex == 0) {
						BT_uint8_buff[BT_uint8_buff_end++] = (uint8_t) interpret2DigitHexAscii(ascii_hex);
						bytes_read_in_this_payload++;
						
						//Serial.print("Tympan: read_BC127_V7_command_mode: completed hex value = ");
						//Serial.print((int)(BT_uint8_buff[BT_uint8_buff_end-1]));
						//Serial.print(", ");
						//Serial.println(BT_uint8_buff[BT_uint8_buff_end-1]);
					}
					
				} else {
					//this is an unexpected character, so ignore everything
					state = STATE__REJECT;
				}
			}
			
			// have we gotten all the bytes?  Are we done?
			if (bytes_read_in_this_payload == n_bytes_to_read) {
				is_bytestream_done = true;
				state = 0; size_str_ind = 0; n_bytes_to_read = 0; //reset
			}
			
		} else {   //this is the STATE__REJECT branch
			//look for carriage return marking the end of this message?
			if ( c == '\r' ) {
				state = 0; size_str_ind = 0; n_bytes_to_read = 0; //reset
			}
		}  // close the if-else block regarding the read state
		
	}  //close the while loop where we read through the incoming buffer
	if (is_bytestream_done) { state = 0;  is_bytestream_done = false; } //we've successfully existied, so reset
} //close the method read_BC127_V7_command_mode()

int TympanBase::interpret2DigitHexAscii(char *str) {
	int ret_val=0; int foo_val=0;
	for (int ind = 0; ind < 2; ind++) {
		if ( (str[ind] >= '0') && (str[ind] <= '9') ) foo_val = (int)(str[ind] - '0');
		if ( (str[ind] >= 'A') && (str[ind] <= 'F') ) foo_val = 10 + (int)(str[ind] - 'A');
		if (ind == 0) ret_val += (16*foo_val);
		if (ind == 1) ret_val += foo_val;
	}
	return ret_val;
}