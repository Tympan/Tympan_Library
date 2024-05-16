#ifndef _BLE_BC127_H
#define _BLE_BC127_H

#include <Arduino.h>   //for Serial
//include <Tympan_Library.h>
#include "ble/ble.h"
#include "ble/bc127.h"
#include <Tympan.h>
#include <SerialManager_UI.h>

class BLE_BC127 : virtual public BLE, public BC127
{
public:
	BLE_BC127(HardwareSerial *sp) : BC127(sp) {}
	BLE_BC127(TympanBase *tympan) : BC127(tympan->BT_Serial) { setPins(tympan->getPin_BT_PIO0(),tympan->getPin_BT_RST()); };
  virtual int begin(void) { return begin(true); }
	virtual int begin(int doFactoryReset);
	virtual void setupBLE() { setupBLE(7); } //default to firmware 7
	virtual void setupBLE(int BT_firmware) { setupBLE(BT_firmware, true); }           //to be called from the Arduino sketch's setup() routine.  Includes factory reset.
  virtual void setupBLE(int BT_firmware, bool printDebug) { setupBLE(BT_firmware, printDebug, true); };            //to be called from the Arduino sketch's setup() routine.  Includes factory reset.
  void setupBLE_noFactoryReset(int BT_firmware = 7, bool printDebug = true);  //to be called from the Arduino sketch's setup() routine.  Excludes factory reset.
	void setupBLE(int BT_firmware, bool printDebug, int doFactoryReset);  //to be called from the Arduino sketch's setup() routine.  Must define all params
	size_t sendByte(char c);
  size_t sendString(const String &s, bool print_debug = false);
  virtual size_t sendMessage(const String &s);
	//size_t sendMessage(const char* c_str, const int len); //use this if you need to send super long strings (more than 1797 characters)
  virtual size_t recvMessage(String *s);
  virtual size_t recvBLE(String *s, bool printResponse = false);
	bool interpretAnyOpenOrClosedMsg(String tmp, bool printDebug=false);
	virtual int isAdvertising(bool printResponse);
	virtual int isAdvertising(void) { return isAdvertising(false); };
  //virtual int isConnected(bool printResponse);
	virtual int isConnected(void) { return BC127::isConnected(false); }
  bool waitConnect(int time = -1);
	virtual void updateAdvertising(unsigned long curTime_millis, unsigned long updatePeriod_millis = 5000) { updateAdvertising(curTime_millis, updatePeriod_millis, false); }
	virtual void updateAdvertising(unsigned long curTime_millis, unsigned long updatePeriod_millis, bool printDebugMsgs);
	
	void echoBTreply(bool printDebug = false);
	bool setUseFasterBaudRateUponBegin(bool enable = true) { return useFasterBaudRateUponBegin = enable; }
	
	virtual int available(void) { return BC127::available(); }
	virtual int read(void) { return BC127::read(); }
	virtual int peek(void) { return BC127::peek(); }
	virtual int version(String &reply) { return BC127::version(reply); }
	virtual int getBleName(String &reply) { return BC127::getBleName(reply); }
protected:
	bool useFasterBaudRateUponBegin = false; //default as to whether to use faster baud rate or not
	void setSerialBaudRate(int new_baud);
	int hardwareFactoryReset(bool printDebug = false);
	void switchToNewBaudRate(int new_baudrate);

};

class BLE_BC127_UI : public BLE_BC127, virtual public BLE_UI
{
	public:
		BLE_BC127_UI(TympanBase *tympan) : BLE_UI(), BLE_BC127(tympan)  {}
		BLE_BC127_UI(HardwareSerial *sp) : BLE_UI(), BLE_BC127(sp) {}

		// ///////// here are the methods that you must implement from SerialManager_UI
		virtual void printHelp(void);
		//virtual bool processCharacter(char c); //not used here
		virtual bool processCharacterTriple(char mode_char, char chan_char, char data_char);
		virtual void setFullGUIState(bool activeButtonsOnly = false) {}; 
		// ///////// end of required methods
		virtual bool processSingleCharacter(char data_char);

};

#endif