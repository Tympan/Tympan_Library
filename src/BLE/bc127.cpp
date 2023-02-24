#include "bc127.h"

#include <Arduino.h>

// Wraps internal stream port
// Returns: int, number of available bytes
int BC127::available()
{
    return _serialPort->available();
}

// Wraps internal stream read
// Returns int, first byte read or -1 if no data available
int BC127::read()
{
    return _serialPort->read();
}

// Wraps internal stream readString
// Returns: String, the string that was read
String BC127::readString(size_t max)
{
    return _serialPort->readString(max);
}

// Sets internal timeout
void BC127::setTimeout(unsigned long timeout)
{
    _timeout = timeout;
}

// Wraps internal stream print
// Returns: size_t, number of bytes written
size_t BC127::print(char c)
{
    return _serialPort->print(c);
}

// Wraps internal stream print
// Returns: size_t, number of bytes written
size_t BC127::printString(const String &s)
{
    return _serialPort->print(s);
}

// Turns on | off BLE advertising
// Returns: SUCCESS | MODULE_ERROR | TIMEOUT_ERROR
BC127::opResult BC127::advertise(bool mode)
{
    String modeStr = mode ? String("ON") : String("OFF");

    return stdCmd("ADVERTISING " + modeStr);
}

// Closes a connection link given by linkid
// Returns: SUCCESS | MODULE_ERROR | TIMEOUT_ERROR
BC127::opResult BC127::close(int linkid)
{
    String linkStr = linkid < 0 ? String("all") : String(linkid);

    return stdCmd("CLOSE " + linkStr);
}

// Power cycles the device
// Returns: SUCCESS | MODULE_ERROR | TIMEOUT_ERROR
BC127::opResult BC127::power(bool mode)
{
    String modeStr = mode ? String("ON") : String("OFF");
    return stdCmd("POWER " + modeStr);
}

// Makes the device discoverable in Classic Mode
// Returns: SUCCESS | MODULE_ERROR | TIMEOUT_ERROR
// Is this V5 firmware only????
BC127::opResult BC127::discoverable(bool mode)
{
    String modeStr = mode ? String("ON") : String("OFF");

    return stdCmd("DISCOVERABLE " + modeStr);
}

// Makes the device discoverable and connectable in Classic Mode (v7 firmware)
// Returns: SUCCESS | MODULE_ERROR | TIMEOUT_ERROR
BC127::opResult BC127::discoverableConnectableV7(bool mode)
{
    String modeStr = mode ? String("ON") : String("OFF");

    return stdCmd("BT_STATE " + modeStr + " " + modeStr);  //example: BT_STATE ON ON
}


// Retrieves the configuration registers - all or specified by config
// Returns: SUCCESS | MODULE_ERROR | TIMEOUT_ERROR
//          _cmdResponse will contain the data on SUCCESS
BC127::opResult BC127::getConfig(String config)
{
    if (config == "")
        return stdCmd("CONFIG");

    BC127::opResult res = stdCmd("GET " + config);

    if (res)
        _cmdResponse = _cmdResponse.substring(config.length() + 1, _cmdResponse.indexOf(EOL));

    return res;
}

// Enters data mode (not available for BLE in firmware v.5.0)
// Returns: SUCCESS | MODULE_ERROR | TIMEOUT_ERROR
BC127::opResult BC127::enterDataMode()
{
    return stdCmd("ENTER_DATA");
}

// Exits from data mode
// Returns: SUCCESS | MODULE_ERROR | TIMEOUT_ERROR
BC127::opResult BC127::exitDataMode(int guardDelay)
{
    delay(guardDelay);

    _serialPort->print("$$$$");
    _serialPort->flush();

    return waitResponse();
}

// Sends a string via the default connection
// Returns: SUCCESS | MODULE_ERROR | TIMEOUT_ERROR
BC127::opResult BC127::send(String str)
{
	if (BC127_firmware_ver >=6) {
		//Assume a link ID of 14
		//  the "14" assumes that we're always sending to the
		//  first ("1", yes they count from "1")  BLE device
		//	that is connected  (BLE connections are the "4")
		if (BLE_id_num >= 14) {
			return stdCmd("SEND " + String(BLE_id_num) + " " + str); 
		} else {
			//Serial.println("BC127: send: *** WARNING *** no BLE connection ID.  Not sending.");
		}
		return MODULE_ERROR; //not really a module error, but we should return an error
	} else {
		return stdCmd("SEND " + str);
	}
}

// Retrieves the device connection status
// Returns: SUCCESS | MODULE_ERROR | TIMEOUT_ERROR
//          _cmdResponse will contain the data on SUCCESS
BC127::opResult BC127::status(bool printResponse)
{
    //return stdCmd("STATUS 14"); //this might be better (more specific to BLE) for V6 firmware and above
	
    BC127::opResult ret_val = stdCmd("STATUS");
	if (printResponse) {
		Serial.print("BC127: status response: ");
		Serial.print(getCmdResponse());  //this should be CR terminated
		if (BC127_firmware_ver > 6) Serial.println();
	}
	return ret_val;	
}

// Resets the device to default configuration
// Returns: SUCCESS | MODULE_ERROR | TIMEOUT_ERROR
//          _cmdResponse contains the data on SUCCESS
BC127::opResult BC127::reset()
{
    return stdCmd("RESET");
}

// Restores a device to factory settings
// Returns: SUCCESS | MODULE_ERROR | TIMEOUT_ERROR
BC127::opResult BC127::restore(bool printResponse)
{
    BC127::opResult ret_val = stdCmd("RESTORE");
	if (printResponse) {
		Serial.print("BC127: restore response: ");
		Serial.print(getCmdResponse());  //this should be CR terminated
		if (BC127_firmware_ver > 6) Serial.println();
	}
	return ret_val;
}

// Writes the current configuration registers to non-volatile memory
// Returns: SUCCESS | MODULE_ERROR | TIMEOUT_ERROR
BC127::opResult BC127::writeConfig()
{
    return stdCmd("WRITE");
}

// Helper function for generic command execution
// Returns: SUCCESS | MODULE_ERROR | TIMEOUT_ERROR
//          _cmdResponse will contain the data on SUCCESS
BC127::opResult BC127::stdCmd(String command)
{
    knownStart(); // Clear the serial buffer in the module and the Arduino.

    // maybe future me can figure out how to send bytes vs. chars
	//for (unsigned int i=0;i < command.length(); i++) _serialPort->write((byte)(command.charAt(i)));
	//for (unsigned int i=0;i < EOC.length(); i++) _serialPort->write((byte)(EOC.charAt(i)));
	//Serial.println("BC12: stdCmd: sending command = " + command + EOC);
	_serialPort->print(command + EOC); //original
    _serialPort->flush();

    return waitResponse();
}

// Sets a given configuration register with param value
// Returns: SUCCESS | MODULE_ERROR | TIMEOUT_ERROR
BC127::opResult BC127::setConfig(String config, String param)
{
    return stdCmd("SET " + config + "=" + param);
}

// Retrieves the firmware version information of the device
// Returns: SUCCESS | MODULE_ERROR | TIMEOUT_ERROR
//          _cmdResponse will contain the data on SUCCESS
BC127::opResult BC127::version(bool printResponse)
{
    BC127::opResult ret_val = stdCmd("VERSION");
	if (printResponse) {
		Serial.print("BC127: version response: ");
		Serial.print(getCmdResponse());  //this should be CR terminated
		if (BC127_firmware_ver > 6) Serial.println();
	}
	return ret_val;
}

// Retrieves the embedded help menu
// Returns: SUCCESS | MODULE_ERROR | TIMEOUT_ERROR
//          _cmdResponse will contain the data on SUCCESS
BC127::opResult BC127::help()
{
    return stdCmd("HELP");
}

// Returns _cmdResponse string from previous command
// Returns: _cmdResponse
String BC127::getCmdResponse()
{
	//Serial.print("BC127: getCmdResponse: _cmdResponse = ");Serial.println(_cmdResponse);
    return _cmdResponse;
}

// Create a known state for the module to start from. If a partial command is
//  already in the module's buffer, we can purge it by sending an EOL to the
//  the module. If not, we'll just get an error.
BC127::opResult BC127::knownStart()
{
    String buffer = "";

    _serialPort->print(EOC);
    _serialPort->flush();

    // We're going to use the internal timer to track the elapsed time since we
    //  issued the reset. Bog-standard Arduino stuff.
    unsigned long startTime = millis();

    // This is our timeout loop. We're going to give our module 100ms to come up
    //  with a new character, and return with a timeout failure otherwise.
    while (buffer.endsWith(EOL) != true)
    {
        // Purge the serial data received from the module, along with any data in
        //  the buffer at the time this command was sent.
        if (_serialPort->available() > 0)
        {
            buffer.concat(char(_serialPort->read()));
            startTime = millis();
        }

        if ((startTime + _timeout) < millis())
            return TIMEOUT_ERROR;
    }

    if (buffer.startsWith("ERR"))
        return SUCCESS;
    else
        return SUCCESS;
}

// Grab an EOL-delimited string of output from the serial line
// Returns: SUCCES | MODULE_ERROR | TIMEOUT_ERROR
//          msg will contain the string on SUCCESS
BC127::opResult BC127::recv(String *msg)
{
    if (!msg) // don't do this
        return MODULE_ERROR;

    // start our timer
    unsigned long startTime = millis();

    // as long as we have time
    while ((startTime + _timeout) > millis())
    {
        // grab a character and append it to the current msg
        if (_serialPort->available() > 0)
        {
            msg->concat((char)_serialPort->read());
        }

        // we've reached the EOL sequence
        if (msg->endsWith(EOL))
        {
            // Lop off the EOL, since it is added by the BC127 and is not part of the characteristic payload
            *msg = msg->substring(0,msg->length()-EOL.length());
            return SUCCESS;
        }
    }

    return TIMEOUT_ERROR;
}

// Convenience method to handle the raw internal stream
// Returns: _serialPort
HardwareSerial *BC127::getSerial()
{
    return _serialPort;
}

// Timeout wait for the response from a command
// Returns: SUCCESS | MODULE_ERROR | TIMEOUT_ERROR
//          _cmdResponse will contain the data on SUCCESS
BC127::opResult BC127::waitResponse(int time)
{
    // some output has multiple lines
    String line = String("");

    // decide how long we're willing to wait
    int timeout = time > 0 ? time : _timeout;

    // get our start time
    unsigned long startTime = millis();

    // reset previous command response
    _cmdResponse = "";

    // as long as we have time
    while ((startTime + timeout) > millis())
    {
        // grab a character and append to current line and command response
        if (_serialPort->available() > 0)
        {
            char temp = _serialPort->read();
            line.concat(temp);
            _cmdResponse.concat(temp);
        }

        // we've reached the end of a line
        if (line.endsWith(EOL))
        {
			//remove leading and trailing whitespace
			line.trim();
			
            // Is this the end of the command response?
            if (line.startsWith("OK") || line.startsWith("Ready"))
            {
                //Serial.println("BC127: waitResponse: SUCCESS! text received = '" + line + "'");
                return SUCCESS;
            }
            else if (line.startsWith("ERROR"))
            {
				//Serial.println("BC127: waitResponse: ERROR! text received = '" + line + "'");
                return MODULE_ERROR;
            }

			//Serial.println("BC127: waitResponse: unknown line. text received = '" + line + "'");

            // move on to next line
            line = "";
        }
    }

    return TIMEOUT_ERROR; // ran out of time
}

int BC127::set_BC127_firmware_ver(int val) { 
	BC127_firmware_ver = max(5,min(7,val));
	if (BC127_firmware_ver < 6) {
		EOL = String("\n\r");
	} else {
		EOL = String("\r");
	}
	return BC127_firmware_ver;
}

//make this a static method so that it can get called without instantiating a BC127 or BLE
int BC127::factoryResetViaPins(int pinPIO0, int pinRST) {
	if ((pinPIO0 < 0) || (pinRST < 0)) return -1;  //FAIL!
	
	//This was all worked by by WEA Aug 25, 2021
	
	pinMode(pinPIO0,OUTPUT);    //prepare the PIO0 pin
	digitalWrite(pinPIO0,HIGH); //normally low.  Swtich high

	pinMode(pinRST,OUTPUT);    //prepare the reset pin
	digitalWrite(pinRST,LOW);  //pull low to reset
	delay(20);                  //hold low for at least 5 msec
	digitalWrite(pinRST,HIGH); //pull high to start the boot

	//wait for boot to proceed far enough before changing anything
	delay(400);                 //V7: works with 200 but not 175.  V5: works with 350 but not 300
	digitalWrite(pinPIO0,LOW); //pull PIO0 back down low
	pinMode(pinPIO0,INPUT);    //go high-impedance to make irrelevant
	
	return 0;
}

bool BC127::isConnected(bool printResponse)
{

    //Ask the BC127 its advertising status.
    //in V5: the reply will be something like: STATE CONNECTED or STATE ADVERTISING
    //in V7: the reply will be something like: STATE CONNECTED[0] CONNECTABLE[OFF] DISCOVERABLE[OFF] BLE[ADVERTISING]
    //   followed by LINK 14 CONNECTED or something like that if the BLE is actually connected to something
    if (status() > 0) //in bc127.cpp.  answer stored in cmdResponse.
    {
		String s = getCmdResponse();  //gets the text reply from the BC127 due to the status() call above
		if (printResponse) {
			Serial.print("BC127: isConnected()   response: ");
			Serial.print(s);
			if (BC127_firmware_ver > 6) Serial.println();
		}
        
		//if (s.indexOf("LINK 14 CONNECTED") == -1) { //if it returns -1, then it wasn't found.  This version is prob better (more specific for BLE) but only would work for V6 and above
		int ind = s.indexOf("CONNECTED");
		if (ind == -1) { //if it returns -1, then it wasn't found.
			if (printResponse) Serial.println("BC127: isConnected: not connected.");
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
					//if (printResponse) Serial.println("BC127 (v7): isConnected: not connected...");				
					return false;
				} else {
					//if (printResponse) Serial.println("BC127: isConnected: yes is connected.");
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
				
				//Serial.print("BC127 (v5x): ind of 'Connected' = ");
				//Serial.println(ind);
				
				//if we got this far, then at least one CONNECTED is seen.  Let's look for IDLE or ADVERTISING, either of which
				//indicate that it's not BLE that is connected
				ind = s.indexOf("IDLE");
				int ind2 = s.indexOf("ADVERTISING");
				if ( (ind >= 0) || (ind2 >= 0) ) { //if either are found, we are not connected
					//Serial.println("BC127: isConnected: found IDLE or ADVERTISING...so NOT connected.");
					//there is IDLE...so, there is no connection
					return false;
				} else {
					//if (printResponse) Serial.println("BC127: isConnected: yes is connected.");
					return true;
				}
			}
		}
    }

	//if we got this far, let's assume that we are not connected
    return false;
}