#ifndef _BLE_H
#define _BLE_H

/* //////////////////////////////////////////////////////////////
*
* BLE
*
* Created: Ray Jenkins, Eric Yuan, Chip Audette
*
* Purpose: Interface for accessing general BLE interactions.
*
* MIT License, Use at your own risk.
*
////////////////////////////////////////////////////////////// */

#include <Arduino.h>   //for Serial
//include "Tympan.h"
#include "SerialManager_UI.h"

class BLE
{
public:
	virtual int begin() = 0; 
	//virtual int begin(int doFactoryReset) = 0; 
	virtual void setupBLE(int) = 0;            //to be called from the Arduino sketch's setup() routine.  Includes factory reset.
  virtual void setupBLE(int, bool) = 0;            //to be called from the Arduino sketch's setup() routine.  Includes factory reset.
  virtual size_t sendString(const String &s, bool print_debug) = 0;  // Send string as raw bytes
  virtual size_t sendMessage(const String &s) = 0;
	//virtual size_t recvMessage(String *s) = 0;
	virtual size_t recvBLE(String *s)=0;
  virtual size_t recvBLE(String *s, bool printResponse) = 0;
	virtual int isAdvertising(void)=0;
  virtual int isConnected(void)=0;
  //virtual bool waitConnect(int time = -1);
	virtual void updateAdvertising(unsigned long curTime_millis, unsigned long updatePeriod_millis)=0;
	virtual void updateAdvertising(unsigned long curTime_millis, unsigned long updatePeriod_millis, bool printDebugMsgs)=0;
	//virtual void echoBTreply(bool printDebug = false);
	virtual bool setUseFasterBaudRateUponBegin(bool enable) = 0;
	
	//receive message, from Tympan Remote App (or otherwise)
	virtual int available(void)=0;
	virtual int read(void)=0;
	virtual int peek(void)=0;
	virtual int version(String *reply_ptr)=0;
	
	virtual int getBleName(String *reply_ptr)=0;
	
protected:
	//virtual bool useFasterBaudRateUponBegin = false; //default as to whether to use faster baud rate or not
	//virtual void setSerialBaudRate(int new_baud);
	//virtual int hardwareFactoryReset(bool printDebug = false);
	//virtual void switchToNewBaudRate(int new_baudrate);

};

class BLE_UI : virtual public BLE, virtual public SerialManager_UI
{
	public:
		//BLE_UI(TympanBase *tympan) : BLE(tympan), SerialManager_UI() {}
		//BLE_UI(HardwareSerial *sp) : BLE(sp), SerialManager_UI() {}

		// ///////// here are the methods that you must implement from SerialManager_UI
		//virtual void printHelp(void)=0;
		// //virtual bool processCharacter(char c)=0; //not used here
		//virtual bool processCharacterTriple(char mode_char, char chan_char, char data_char)=0;
		//virtual void setFullGUIState(bool activeButtonsOnly)=0; 
		// ///////// end of required methods
		//virtual bool processSingleCharacter(char data_char)=0;

};

#endif