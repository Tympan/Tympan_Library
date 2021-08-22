#ifndef _BLE_H
#define _BLE_H

#include <Arduino.h>   //for Serial
//include <Tympan_Library.h>
#include "bc127.h"
#include "Tympan.h"

class BLE : public BC127
{
public:
    BLE(Stream *sp) : BC127(sp) {}
	int begin(void);
	void setupBLE(int BT_firmware = 7, bool printDebug = true);  //to be called from the Arduino sketch's setup() routine.  Includes error reporting to Serial
    size_t sendByte(char c);
    size_t sendString(const String &s);
    size_t sendMessage(const String &s);
	//size_t sendMessage(const char* c_str, const int len); //use this if you need to send super long strings (more than 1797 characters)
    size_t recvMessage(String *s);
    size_t recvBLE(String *s, bool printResponse = false);
	bool interpretAnyOpenOrClosedMsg(String tmp, bool printDebug=false);
	bool isAdvertising(bool printResponse = false);
    bool isConnected(bool printResponse = false);
    bool waitConnect(int time = -1);
	void updateAdvertising(unsigned long curTime_millis, unsigned long updatePeriod_millis = 5000, bool printDebugMsgs=false);

};

class BLE_UI : public BLE, public SerialManager_UI
{
	public:
		BLE_UI(HardwareSerial *sp) : BLE(sp), SerialManager_UI() {}

		// ///////// here are the methods that you must implement from SerialManager_UI
		virtual void printHelp(void);
		//virtual bool processCharacter(char c); //not used here
		virtual bool processCharacterTriple(char mode_char, char chan_char, char data_char);
		virtual void setFullGUIState(bool activeButtonsOnly = false) {}; 
		// ///////// end of required methods
		virtual bool processCharacter(char data_char);

};

#endif